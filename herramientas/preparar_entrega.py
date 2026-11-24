#!/usr/bin/env python3
"""Verifica y empaqueta fuentes CanSAT; nunca carga placas ni instala dependencias."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
FILES = [
    'TELEMETRIA.md', 'estacion_terrena.html',
    'firmware/telemetria/README.md', 'firmware/telemetria/GUIA_COMPANERO.md',
    'firmware/telemetria/transmisor/transmisor.ino',
    'firmware/telemetria/transmisor/paquete.h', 'firmware/telemetria/transmisor/ciclo.h',
    'firmware/telemetria/receptor/receptor.ino',
    'firmware/telemetria/receptor/contrato.h', 'firmware/telemetria/receptor/ciclo.h',
    'herramientas/preparar_entrega.py', 'pruebas/test_entrega.py',
    'pruebas/test_firmware.cpp', 'pruebas/test_transmisor.cpp', 'pruebas/test_receptor.cpp',
    'pruebas/dobles/Arduino.h', 'pruebas/dobles/SPI.h', 'pruebas/dobles/LoRa.h',
    'pruebas/firmware_frontend.cjs', 'pruebas/dashboard_qa.cjs',
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, obj):
    path.write_text(json.dumps(obj, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def snapshot(root):
    for name in FILES:
        p = root / name
        if not p.is_file() or p.is_symlink() or not p.resolve().is_relative_to(root.resolve()):
            raise ValueError(f'Fuente ausente o fuera de la entrega: {name}')
    return {name: digest(root / name) for name in FILES}


def check_manifest(root):
    manifest = json.loads((root / 'MANIFIESTO.json').read_text(encoding='utf-8'))
    hashes = manifest['archivos_sha256']
    if set(hashes) != set(FILES + ['INFORME_VERIFICACION.json']):
        raise ValueError('El manifiesto no contiene el inventario obligatorio exacto')
    for name, expected in hashes.items():
        p = root / name
        if p.is_symlink() or not p.resolve().is_relative_to(root.resolve()) or not p.is_file():
            raise ValueError(f'Archivo inválido: {name}')
        if digest(p) != expected:
            raise ValueError(f'Integridad incorrecta: {name}')
    actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
    if actual != set(hashes) | {'MANIFIESTO.json'}:
        raise ValueError('Hay archivos inesperados o ausentes en la entrega')
    return manifest


def verify(args, root, output):
    output.mkdir(parents=True, exist_ok=True)
    report = {'verificado_en_computadora': False, 'fecha_utc': datetime.now(timezone.utc).isoformat(),
              'placa': 'esp32:esp32:esp32', 'versiones': {}, 'pasos': [],
              'pendientes_fisicos': ['carga USB', 'cableado', 'sensores', 'recepcion LoRa', 'alcance', 'vuelo']}
    target = output / 'informe.json'
    save(target, report)  # Un fallo nuevo nunca deja como vigente un informe aprobado anterior.
    env = {**os.environ, 'PYTHONDONTWRITEBYTECODE': '1'}

    def run(label, cmd, stdin=None):
        print(label, flush=True)
        entry = {'prueba': label, 'comando': list(map(str, cmd)), 'resultado': 'ERROR'}
        report['pasos'].append(entry)
        try:
            p = subprocess.run(cmd, cwd=root, input=stdin, text=True, env=env,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=900)
            logfile = output / f'paso_{len(report["pasos"]):02d}.log'
            logfile.write_text(p.stdout, encoding='utf-8')
            entry.update(codigo=p.returncode, salida=p.stdout[-6000:], log=logfile.name)
            if p.returncode:
                raise RuntimeError(f'{label}: código {p.returncode}; consultar {logfile}')
            entry['resultado'] = 'PASA'
            save(target, report)
            return p.stdout
        except (OSError, subprocess.TimeoutExpired) as e:
            entry['error'] = str(e)
            raise RuntimeError(f'{label}: {e}') from e

    try:
        if (root / 'MANIFIESTO.json').exists():
            check_manifest(root)
        before = snapshot(root)
        tools = {}
        for key, value in [('cli', args.arduino_cli), ('cxx', args.cxx), ('node', args.node)]:
            found = shutil.which(value)
            if not found:
                raise ValueError(f'Dependencia no disponible: {value}')
            tools[key] = str(Path(found).resolve())
        lib = Path(args.lora).resolve()
        props = dict(line.split('=', 1) for line in (lib / 'library.properties').read_text().splitlines()
                     if '=' in line and not line.startswith('#'))
        if props.get('name') != 'LoRa' or props.get('version') != '0.8.0' or 'Sandeep Mistry' not in props.get('author', ''):
            raise ValueError('Se requiere LoRa 0.8.0 de Sandeep Mistry')
        report['versiones']['LoRa'] = props['version']
        report['versiones']['arduino-cli'] = json.loads(run('Version Arduino CLI', [tools['cli'], 'version', '--json']))['VersionString']
        cores = json.loads(run('Version core ESP32', [tools['cli'], 'core', 'list', '--json']))
        core = next((p.get('installed_version') for p in cores['platforms'] if p['id'] == 'esp32:esp32'), None)
        if core != '3.3.12':
            raise ValueError(f'Se requiere core ESP32 3.3.12; encontrado: {core}')
        report['versiones']['core_esp32'] = core
        report['versiones']['g++'] = run('Version compilador nativo', [tools['cxx'], '--version']).splitlines()[0]
        report['versiones']['node'] = run('Version Node', [tools['node'], '--version']).strip()
        binaries = {}
        for name in ['firmware', 'transmisor', 'receptor']:
            binary = output / f'test_{name}'
            run(f'Compilar prueba {name}', [tools['cxx'], '-std=c++11', '-Wall', '-Wextra', '-Werror',
                '-pedantic', '-I', str(root / 'pruebas/dobles'), str(root / f'pruebas/test_{name}.cpp'), '-o', str(binary)])
            run(f'Ejecutar prueba {name}', [str(binary)])
            binaries[name] = binary
        for mode, script_args in [('--emit', []), ('--emit-flight', [])]:
            data = run(f'Fixture artificial {mode}', [str(binaries['firmware']), mode])
            run(f'Frontend {mode}', [tools['node'], str(root / 'pruebas/firmware_frontend.cjs'), *script_args], data)
        data = run('Capturar receptor real con dobles', [str(binaries['receptor']), '--emit'])
        run('Receptor a frontend', [tools['node'], str(root / 'pruebas/firmware_frontend.cjs'), '--receptor'], data)
        run('QA frontend 20 casos', [tools['node'], str(root / 'pruebas/dashboard_qa.cjs')])
        run('Pruebas de entrega y dependencias ausentes', [sys.executable, str(root / 'pruebas/test_entrega.py')])
        for name in ['transmisor', 'receptor']:
            run(f'Compilar ESP32 {name}', [tools['cli'], 'compile', '--fqbn', 'esp32:esp32:esp32',
                '--library', str(lib), '--build-path', str(output / f'build_{name}'), '--jobs', '2',
                str(root / f'firmware/telemetria/{name}')])
        if before != snapshot(root):
            raise ValueError('Las fuentes cambiaron durante la verificación; repetir desde una versión estable')
        report.update(verificado_en_computadora=True, fuentes_sha256=before)
    except (ValueError, RuntimeError, OSError, KeyError) as e:
        report['error'] = str(e)
    save(target, report)
    return report


def package(args, output):
    final = output / 'cansat-telemetria-v2.zip'
    if final.exists():
        raise ValueError('La entrega ya existe; usar otra carpeta para conservar la versión previa')
    initial = verify(args, ROOT, output / 'verificacion_origen')
    if not initial['verificado_en_computadora']:
        raise ValueError(initial['error'])
    # Crear y verificar un ZIP candidato antes de publicarlo como entrega final.
    with tempfile.TemporaryDirectory(prefix='cansat-entrega-', dir=output) as temp:
        temp = Path(temp)
        stage = temp / 'fuentes'
        stage.mkdir()
        for name in FILES:
            dest = stage / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, dest)
        if snapshot(stage) != initial['fuentes_sha256']:
            raise ValueError('Las fuentes empaquetadas difieren de las verificadas')
        shutil.copyfile(output / 'verificacion_origen/informe.json', stage / 'INFORME_VERIFICACION.json')
        ref = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True, capture_output=True)
        status = subprocess.run(['git', 'status', '--porcelain'], cwd=ROOT, text=True, capture_output=True)
        save(stage / 'MANIFIESTO.json', {'versiones': initial['versiones'],
            'referencia_git': ref.stdout.strip() if ref.returncode == 0 else None,
            'arbol_modificado': bool(status.stdout) if status.returncode == 0 else None,
            'nota': 'Las huellas identifican la entrega; la referencia Git no sustituye el árbol local.',
            'archivos_sha256': {n: digest(stage/n) for n in FILES + ['INFORME_VERIFICACION.json']}})
        candidate = temp / 'candidato.zip'
        with zipfile.ZipFile(candidate, 'x', zipfile.ZIP_DEFLATED) as z:
            for p in sorted(stage.rglob('*')):
                if p.is_file():
                    z.write(p, p.relative_to(stage))
        extracted = temp / 'extraido'
        with zipfile.ZipFile(candidate) as z:
            z.extractall(extracted)  # Archivo creado arriba desde la lista explícita, sin entradas externas.
        check_manifest(extracted)
        cmd = [sys.executable, str(extracted/'herramientas/preparar_entrega.py'), 'verificar',
               '--arduino-cli', args.arduino_cli, '--lora', str(Path(args.lora).resolve()),
               '--cxx', args.cxx, '--node', args.node, '--salida', str(output/'verificacion_extraida')]
        result = subprocess.run(cmd, cwd=extracted, env={**os.environ, 'PYTHONDONTWRITEBYTECODE':'1'}, timeout=1800)
        if result.returncode:
            raise ValueError('Falló la verificación extraída: no se publica el ZIP final')
        check_manifest(extracted)
        with final.open('xb') as target, candidate.open('rb') as source:
            shutil.copyfileobj(source, target)
    save(output / 'entrega.json', {'verificado_en_computadora': True, 'zip': final.name,
        'sha256': digest(final), 'verificacion_extraida': 'verificacion_extraida/informe.json',
        'pendientes_fisicos': initial['pendientes_fisicos']})
    return final


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('accion', choices=['verificar', 'empaquetar', 'integridad'])
    parser.add_argument('--arduino-cli', default='arduino-cli')
    parser.add_argument('--lora', help='Carpeta de LoRa 0.8.0 con library.properties')
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--node', default='node')
    parser.add_argument('--salida', type=Path, help='Carpeta de registros/builds, fuera de fuentes si hay manifiesto')
    args = parser.parse_args()
    try:
        if args.accion == 'integridad':
            check_manifest(ROOT)
            print('Integridad del paquete: OK')
            return 0
        if not args.lora or not args.salida:
            parser.error('--lora y --salida son obligatorios para verificar/empaquetar')
        output = args.salida.resolve()
        if (ROOT/'MANIFIESTO.json').exists() and output.is_relative_to(ROOT):
            raise ValueError('Guardar los registros fuera de las fuentes manifestadas')
        output.mkdir(parents=True, exist_ok=True)
        if args.accion == 'empaquetar':
            print(package(args, output))
            return 0
        report = verify(args, ROOT, output)
        print('Verificación: OK' if report['verificado_en_computadora'] else 'Verificación: ERROR: '+report['error'])
        return 0 if report['verificado_en_computadora'] else 1
    except (ValueError, OSError, subprocess.TimeoutExpired) as e:
        print(f'ERROR: {e}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
