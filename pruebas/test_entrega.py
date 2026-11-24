"""Comprueba dependencia ausente y detección de alteraciones sin placa ni red."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('entrega', ROOT/'herramientas/preparar_entrega.py')
entrega = importlib.util.module_from_spec(spec)
spec.loader.exec_module(entrega)


class EntregaTests(unittest.TestCase):
    def test_missing_cli_is_not_success(self):
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp)/'salida'
            output.mkdir()
            # Un resultado previo aprobado no debe sobrevivir a una ejecución fallida.
            (output/'informe.json').write_text('{"verificado_en_computadora":true}')
            p = subprocess.run([sys.executable, str(ROOT/'herramientas/preparar_entrega.py'),
                'verificar', '--arduino-cli', str(Path(tmp)/'cli-inexistente'),
                '--lora', tmp, '--salida', str(output)], text=True, capture_output=True)
            self.assertNotEqual(p.returncode, 0)
            report = json.loads((output/'informe.json').read_text())
            self.assertFalse(report['verificado_en_computadora'])
            self.assertIn('Dependencia no disponible', report['error'])

    def test_manifest_detects_changed_missing_and_unexpected_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in entrega.FILES+['INFORME_VERIFICACION.json']:
                p = root/name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text('fixture')
            hashes = {name: entrega.digest(root/name) for name in entrega.FILES+['INFORME_VERIFICACION.json']}
            entrega.save(root/'MANIFIESTO.json', {'archivos_sha256': hashes})
            entrega.check_manifest(root)
            item = root/'TELEMETRIA.md'
            item.write_text('alterado')
            with self.assertRaises(ValueError): entrega.check_manifest(root)
            item.write_text('fixture')
            item.unlink()
            with self.assertRaises(ValueError): entrega.check_manifest(root)
            item.write_text('fixture')
            (root/'secreto-no-incluido.txt').write_text('fixture')
            with self.assertRaises(ValueError): entrega.check_manifest(root)


if __name__ == '__main__':
    unittest.main()
