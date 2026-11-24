"""Casos de contrato y análisis; ejecutar desde la raíz con unittest discover."""
import unittest

from herramientas.telemetria import UINT32, analyze, parse_line, simulated_lines


def packet(**changes):
    fields = list(parse_line(next(simulated_lines())))
    values = parse_line(next(simulated_lines()))
    values.update(changes)
    return '$' + ','.join(str(values[k]) for k in fields)


class TelemetriaTests(unittest.TestCase):
    def test_example_from_contract(self):
        d = parse_line('$1204,60400,1,986.12,26.4,310.52,0.02,0.01,1.00,3.5,-2.1,141,0,-1,-1,-71,8.5')
        self.assertEqual((d['seq'], d['st'], d['sd']), (1204, 1, -1))

    def test_reject_malformed_packets(self):
        good = packet()
        for bad in [good[1:], good + ',0', good.rsplit(',', 1)[0],
                    packet(temp=''), packet(temp='NaN'), packet(temp='Infinity'),
                    packet(temp='1e999'), packet(st=6), packet(st=1.5),
                    packet(seq=-1), packet(ms=UINT32), packet(flags=16),
                    packet(sd=-2), packet(sb=360)]:
            with self.subTest(line=bad), self.assertRaises(ValueError):
                parse_line(bad)

    def test_crlf_and_signed_measurements(self):
        d = parse_line(packet(temp=-10, roll=-1, ay=-1) + '\r\n')
        self.assertEqual(d['ay'], -1)

    def test_complete_simulated_flight(self):
        report, rows = analyze(simulated_lines())
        self.assertEqual(report['validos'], 386)
        self.assertEqual({d['st'] for d in rows}, set(range(6)))
        s = report['sesiones'][0]
        self.assertEqual(s['perdidos_estimados'], 0)
        self.assertEqual(s['altura_max_m'], 450)
        self.assertEqual(s['post_aterrizaje_s'], 12)
        self.assertAlmostEqual(rows[320]['vel'], -10, places=5)

    def test_known_packet_losses(self):
        report, _ = analyze(simulated_lines(drop_every=10))
        s = report['sesiones'][0]
        self.assertEqual(s['recibidos'], 348)
        self.assertEqual(s['perdidos_estimados'], 38)

    def test_duplicate_and_late_packet_do_not_corrupt_derivatives(self):
        report, rows = analyze([packet(seq=100, ms=20000, alt=20),
                                packet(seq=100, ms=20000, alt=20),
                                packet(seq=99, ms=19800, alt=500),
                                packet(seq=101, ms=20200, alt=18)])
        self.assertEqual(report['duplicados'], 1)
        self.assertEqual(report['fuera_de_orden'], 1)
        self.assertEqual(len(rows), 2)
        self.assertAlmostEqual(rows[-1]['vel'], -10)
        self.assertEqual(report['sesiones'][0]['altura_max_m'], 20)

    def test_observable_restart_starts_new_session(self):
        report, _ = analyze([packet(seq=100, ms=20000, alt=100), packet(seq=0, ms=0)])
        self.assertEqual(len(report['sesiones']), 2)
        self.assertEqual(report['sesiones'][1]['perdidos_estimados'], 0)

    def test_uint32_wrap_is_not_restart(self):
        report, rows = analyze([packet(seq=UINT32-1, ms=UINT32-100, alt=10),
                                packet(seq=0, ms=100, alt=8)])
        self.assertEqual(len(report['sesiones']), 1)
        self.assertEqual(report['sesiones'][0]['perdidos_estimados'], 0)
        self.assertAlmostEqual(rows[-1]['vel'], -10)

    def test_invalid_packet_does_not_block_next_valid(self):
        report, rows = analyze(['debug: arrancando', packet(), packet(temp='NaN'),
                                packet(seq=1, ms=200)])
        self.assertEqual(report['invalidos'], 2)
        self.assertEqual(len(rows), 2)

    def test_no_landing_evidence(self):
        report, _ = analyze([packet()])
        self.assertIsNone(report['sesiones'][0]['post_aterrizaje_s'])

    def test_missing_altitude_does_not_create_velocity(self):
        report, rows = analyze([packet(alt=-1), packet(seq=1, ms=200, alt=10)])
        self.assertIsNone(rows[-1]['vel'])
        self.assertEqual(report['sesiones'][0]['altura_max_m'], 10)


if __name__ == '__main__':
    unittest.main()
