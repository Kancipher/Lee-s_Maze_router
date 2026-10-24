"""CLI regressions plus an independent Python shortest-path oracle."""
import heapq
import pathlib
import random
import re
import subprocess
import sys
import tempfile
import unittest

BINARY = str(pathlib.Path(sys.argv.pop(1)).resolve())


def oracle(rows, cols, obstacles, start, goal, via, direction):
    queue, best = [(0, start)], {start: 0}
    while queue:
        cost, p = heapq.heappop(queue)
        if cost != best[p]:
            continue
        if p == goal:
            return cost
        l, x, y = p
        for q, weight in [((l,x-1,y),1 if l==1 else direction), ((l,x+1,y),1 if l==1 else direction),
                          ((l,x,y-1),1 if l==2 else direction), ((l,x,y+1),1 if l==2 else direction), ((3-l,x,y),via)]:
            ll, xx, yy = q
            if not (0 <= xx < cols and 0 <= yy < rows) or (xx,yy) in obstacles:
                continue
            if cost+weight < best.get(q, float('inf')):
                best[q] = cost+weight
                heapq.heappush(queue,(cost+weight,q))
    return None


class RouterTests(unittest.TestCase):
    def run_case(self, text, via=10, direction=10, existing=None):
        with tempfile.TemporaryDirectory() as folder:
            inp, out = pathlib.Path(folder)/'in.txt', pathlib.Path(folder)/'out.txt'
            inp.write_text(text)
            if existing is not None:
                out.write_text(existing)
            result = subprocess.run([BINARY,str(inp),str(out),str(via),str(direction)],capture_output=True,text=True,timeout=15)
            return result, out.read_text() if out.exists() else None

    def test_random_optimal_cost_and_path_invariants(self):
        rng = random.Random(2026)
        for i in range(80):
            rows,cols = rng.randint(2,9),rng.randint(2,11)
            start,goal = (1,0,0),(rng.choice([1,2]),cols-1,rows-1)
            obs={(x,y) for x in range(cols) for y in range(rows) if rng.random()<.22} - {(0,0),(cols-1,rows-1)}
            via,direction=rng.randint(1,15),rng.randint(1,12)
            text=f'{rows}x{cols}\n'+''.join(f'OBS ({x},{y})\n' for x,y in sorted(obs))+f'netA {start}{goal}\n'
            with self.subTest(case=i):
                result,output=self.run_case(text,via,direction)
                expected=oracle(rows,cols,obs,start,goal,via,direction)
                self.assertEqual(result.returncode,2 if expected is None else 0,result.stderr)
                if expected is None:
                    self.assertEqual(output,'netA UNROUTABLE\n')
                    continue
                self.assertEqual(int(re.search(r'cost=(\d+)',result.stdout)[1]),expected)
                cells={tuple(map(int,p)) for p in re.findall(r'\((\d+),(\d+),(\d+)\)',output)}
                self.assertTrue({start,goal} <= cells)
                self.assertTrue(all((x,y) not in obs and 0<=x<cols and 0<=y<rows for l,x,y in cells))
                seen={start};todo=[start]
                while todo:
                    l,x,y=todo.pop()
                    for n in [(l,x-1,y),(l,x+1,y),(l,x,y-1),(l,x,y+1),(3-l,x,y)]:
                        if n in cells and n not in seen:
                            seen.add(n);todo.append(n)
                self.assertEqual(seen,cells)

    def test_invalid_inputs_do_not_create_output(self):
        cases=['0x3\nnet0 (1,0,0)(1,1,1)', '999999x999999\n', '2x3\nnet0 (3,0,0)(1,1,1)',
               '2x3\nnet0 (1,-1,0)(1,1,1)', '2x3\nnet0 (1,3,0)(1,1,1)',
               '2x3\nnet0 (1,0,0)', '2x3\nnet0 (1,0,0)(1,0,0)',
               '2x3\nOBS (0,0)\nnet0 (1,0,0)(1,1,1)', '2x3\nnet0 (1,0,0)(1,1,1) garbage',
               '2x3\nnet0 (1,0,0)(1,1,1)\nnet0 (2,0,0)(2,1,1)', '2x3\nunknown', '2x3\nnet0 (1,9999999999999999999999,0)(1,1,1)', '2x3\n'+ ' ' * 5000]
        for text in cases:
            with self.subTest(text=text):
                result,output=self.run_case(text)
                self.assertEqual(result.returncode,1,result.stderr)
                self.assertIsNone(output)

    def test_cost_validation(self):
        for via,direction in [(0,10),(-1,1),(1,0),(1000001,10)]:
            result,output=self.run_case('2x3\nnet0 (1,0,0)(2,2,1)',via,direction)
            self.assertEqual(result.returncode,1);self.assertIsNone(output)

    def test_names_multipin_and_shared_tree(self):
        result,output=self.run_case('4x9\nnetLong (1,0,0)(1,8,0)(1,4,0)\nnetShort (2,2,3)(2,3,3)')
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertTrue(output.splitlines()[0].startswith('netLong '))
        self.assertTrue(output.splitlines()[1].startswith('netShort '))
        self.assertIn('netLong: ROUTED cost=8 cells=9',result.stdout)
        sets=[set(re.findall(r'\([^)]*\)',line)) for line in output.splitlines()]
        self.assertFalse(sets[0]&sets[1])

    def test_failed_multipin_has_no_exported_partial_track(self):
        result,output=self.run_case('3x4\nOBS (2,0)\nOBS (2,1)\nOBS (2,2)\nnet0 (1,0,0)(1,1,0)(1,3,0)')
        self.assertEqual(result.returncode,2);self.assertEqual(output,'net0 UNROUTABLE\n')

    def test_output_preserved(self):
        result,output=self.run_case('2x3\nnet0 (1,0,0)(2,2,1)',existing='keep me')
        self.assertEqual(result.returncode,1);self.assertEqual(output,'keep me')

    def test_historical_fixtures_are_handled_without_crash(self):
        for p in pathlib.Path(__file__).parent.glob('Testcase*/input.txt'):
            result,_=self.run_case(p.read_text())
            self.assertIn(result.returncode,(0,2),p.name)
            self.assertNotIn('Sanitizer',result.stderr)


if __name__=='__main__':
    unittest.main()
