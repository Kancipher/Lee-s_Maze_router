"""Exercise persistent extension state across successful and failed calls."""
import pathlib
import tempfile
import unittest
import routing


class BindingTests(unittest.TestCase):
    def test_repeated_calls_and_invalid_input_are_isolated(self):
        with tempfile.TemporaryDirectory() as d:
            d=pathlib.Path(d)
            inp=d/'input.txt'
            inp.write_text('3x8\nnetFirst (1,0,0)(1,7,0)')
            routing.set_costs(10,10)
            routing.readfile(str(inp),str(d/'first.txt'))
            self.assertEqual([r['name'] for r in routing.get_results()],['netFirst'])
            inp.write_text('2x3\nnetSecond (1,0,0)(2,2,1)')
            routing.readfile(str(inp),str(d/'second.txt'))
            self.assertEqual([r['name'] for r in routing.get_results()],['netSecond'])
            self.assertEqual((len(routing.get_grid()[0]),len(routing.get_grid()[0][0])),(3,2))
            before=routing.get_grid()
            inp.write_text('2x3\nnetBad (1,-1,0)(2,2,1)')
            with self.assertRaises(ValueError):routing.readfile(str(inp),str(d/'invalid.txt'))
            self.assertFalse((d/'invalid.txt').exists())
            self.assertEqual(routing.get_grid(),before)
            with self.assertRaises(ValueError):routing.set_costs(-1,10)

    def test_unroutable_tree_leaves_only_original_pins(self):
        with tempfile.TemporaryDirectory() as d:
            inp=pathlib.Path(d)/'input.txt';out=pathlib.Path(d)/'out.txt'
            inp.write_text('3x4\nOBS (2,0)\nOBS (2,1)\nOBS (2,2)\nnet0 (1,0,0)(1,1,0)(1,3,0)')
            routing.readfile(str(inp),str(out))
            self.assertFalse(routing.get_results()[0]['routed'])
            self.assertFalse(any(value in (2,3) for layer in routing.get_grid() for col in layer for value in col))
            self.assertFalse(any(name for layer in routing.get_net_name_grid() for col in layer for name in col))


if __name__=='__main__':unittest.main()
