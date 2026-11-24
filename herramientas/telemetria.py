#!/usr/bin/env python3
"""Pruebas de telemetría v1, sin dependencias externas ni acceso a radio."""
import argparse
import csv
import json
import math
from pathlib import Path
import re

FIELDS = 'seq ms st pres temp alt ax ay az roll pitch yaw flags sd sb rssi snr'.split()
NUMBER = re.compile(r'[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?\Z')
UINT32 = 2**32


def parse_line(line):
    line = line.strip()
    if not line.startswith('$'):
        raise ValueError('falta el prefijo $')
    tokens = line[1:].split(',')
    if len(tokens) != len(FIELDS):
        raise ValueError(f'se esperaban 17 campos; llegaron {len(tokens)}')
    if any(not NUMBER.fullmatch(t) for t in tokens):
        raise ValueError('campo vacío o número inválido')
    values = list(map(float, tokens))
    if not all(math.isfinite(v) for v in values):
        raise ValueError('número no finito')
    d = dict(zip(FIELDS, values))
    for key, maximum in [('seq', UINT32 - 1), ('ms', UINT32 - 1), ('st', 5), ('flags', 15)]:
        if not d[key].is_integer() or not 0 <= d[key] <= maximum:
            raise ValueError(f'{key}: entero fuera de rango')
        d[key] = int(d[key])
    if d['sd'] != -1 and d['sd'] < 0:
        raise ValueError('sd: debe ser -1 o una distancia no negativa')
    if d['sb'] != -1 and not 0 <= d['sb'] < 360:
        raise ValueError('sb: debe ser -1 o un rumbo en [0,360)')
    return d


def simulated_lines(drop_every=0):
    """Vuelo sintético a 5 Hz: 450 m, separación 200 m, aterrizaje T+65 s."""
    for seq in range(386):
        t = seq / 5
        if t < 5:
            st, alt = 0, 0
        elif t < 35:
            st, alt = 1, (t - 5) * 15
        elif t < 45:
            st, alt = 2, 450 - (t - 35) * 25
        elif t < 46:
            st, alt = 3, 200 - (t - 45) * 10
        elif t < 65:
            st, alt = 4, 190 - (t - 46) * 10
        else:
            st, alt = 5, 0
        flags = (1 if st >= 3 else 0) | (2 if st >= 4 else 0) | (4 if st == 5 else 0)
        values = [seq, seq * 200, st, round(1013.25 * (1 - alt / 44330)**5.255, 2),
                  25, round(alt, 2), 0, 0, 0.05 if st == 2 else 1,
                  0, 0, 0, flags, -1, -1, -75, 8.5]
        if drop_every and seq and seq % drop_every == 0:
            continue
        yield '$' + ','.join(map(str, values))


def analyze(lines):
    report = {'validos': 0, 'invalidos': 0, 'duplicados': 0, 'fuera_de_orden': 0,
              'sesiones': [], 'errores': []}
    rows = []
    session = None
    previous = None
    elapsed = 0
    landing = None
    velocity = None
    for number, line in enumerate(lines, 1):
        if not line.strip():
            continue
        try:
            d = parse_line(line)
        except ValueError as exc:
            report['invalidos'] += 1
            # Resumen acotado; la captura original conserva toda la evidencia.
            if len(report['errores']) < 20:
                report['errores'].append({'linea': number, 'error': str(exc)})
            continue
        report['validos'] += 1
        # Reinicios ambiguos no se adivinan: ambos contadores deben retroceder
        # cerca del arranque. Un wrap uint32 se trata por aritmética modular.
        restart = previous and d['seq'] < previous['seq'] and d['ms'] < previous['ms'] and d['seq'] < 10 and d['ms'] < 2000 and previous['seq'] < UINT32 // 2
        if previous and not restart:
            ds = (d['seq'] - previous['seq']) % UINT32
            dt = (d['ms'] - previous['ms']) % UINT32
            if ds == 0:
                report['duplicados'] += 1
                continue
            if ds >= UINT32 // 2 or dt == 0 or dt >= UINT32 // 2:
                report['fuera_de_orden'] += 1
                continue
        if previous is None or restart:
            session = {'sesion': len(report['sesiones']) + 1, 'recibidos': 0,
                       'perdidos_estimados': 0, 'altura_max_m': None,
                       'post_aterrizaje_s': None, 'intervalo_max_s': 0}
            report['sesiones'].append(session)
            previous = None
            elapsed, landing, velocity = 0, None, None
        else:
            session['perdidos_estimados'] += ds - 1
            elapsed += dt / 1000
            session['intervalo_max_s'] = max(session['intervalo_max_s'], dt / 1000)
        # -1 es el marcador acordado de dato ausente; no convertirlo en medida.
        if d['alt'] == -1 or previous is None or previous['alt'] == -1:
            velocity = None
        else:
            raw = (d['alt'] - previous['alt']) / (dt / 1000)
            velocity = raw if velocity is None else velocity * .6 + raw * .4
        if d['st'] == 5 and landing is None:
            landing = elapsed
        if landing is not None and d['st'] == 5:
            session['post_aterrizaje_s'] = round(elapsed - landing, 3)
        if d['alt'] != -1:
            maximum = session['altura_max_m']
            session['altura_max_m'] = d['alt'] if maximum is None else max(maximum, d['alt'])
        session['recibidos'] += 1
        rows.append({**d, 'sesion': session['sesion'], 'tiempo_s': elapsed,
                     'vel': velocity,
                     'g': None if -1 in (d['ax'], d['ay'], d['az']) else math.hypot(d['ax'], d['ay'], d['az'])})
        previous = d
    for s in report['sesiones']:
        total = s['recibidos'] + s['perdidos_estimados']
        s['perdida_pct'] = round(100 * s['perdidos_estimados'] / total, 3)
    return report, rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    sim = commands.add_parser('simular', help='generar una captura claramente sintética')
    sim.add_argument('--salida', type=Path, required=True)
    sim.add_argument('--perder-cada', type=int, default=0)
    check = commands.add_parser('verificar', help='validar captura USB de texto, una línea por paquete')
    check.add_argument('captura', type=Path)
    check.add_argument('--csv', type=Path)
    check.add_argument('--informe', type=Path)
    args = parser.parse_args()
    if args.command == 'simular':
        if args.perder_cada < 0:
            parser.error('--perder-cada debe ser no negativo')
        args.salida.write_text('\n'.join(simulated_lines(args.perder_cada)) + '\n', encoding='utf-8')
        print(f'Captura SINTÉTICA: {args.salida}; no demuestra funcionamiento del hardware.')
        return 0
    with args.captura.open(encoding='utf-8', errors='replace') as source:
        report, rows = analyze(source)
    if args.csv:
        with args.csv.open('w', newline='', encoding='utf-8') as target:
            writer = csv.DictWriter(target, fieldnames=FIELDS + ['sesion', 'tiempo_s', 'vel', 'g'])
            writer.writeheader()
            writer.writerows(rows)
    output = json.dumps(report, ensure_ascii=False, indent=2)
    if args.informe:
        args.informe.write_text(output + '\n', encoding='utf-8')
    print(output)
    return 1 if report['invalidos'] or not rows else 0


if __name__ == '__main__':
    raise SystemExit(main())
