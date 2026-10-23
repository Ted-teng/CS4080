# ch 20 challenge 3 (save repeated measurements and their environment)
import csv
import io
import json
import platform
import statistics
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parent
programs = ['bench_lookup', 'bench_strings', 'bench_churn']
subprocess.run(['make', *programs], cwd=root, check=True)
result_dir = root / 'benchmark_results'
result_dir.mkdir(exist_ok=True)
rows = []
for trial in range(5):
    order = programs if trial % 2 == 0 else list(reversed(programs))
    for program in order:
        path = root / (program + ('.exe' if platform.system() == 'Windows' else ''))
        output = subprocess.check_output([str(path)], cwd=root, text=True)
        for row in csv.DictReader(io.StringIO(output)):
            row['trial'] = trial + 1
            row['program'] = program
            rows.append(row)
with (result_dir / 'raw.csv').open('w', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
groups = {}
for row in rows:
    key = (row['workload'], int(row['keys']))
    groups.setdefault(key, []).append(row)
with (result_dir / 'medians.csv').open('w', newline='') as f:
    fields = ['workload', 'keys', 'operations', 'median_seconds', 'median_ns_per_operation', 'capacity']
    writer = csv.DictWriter(f, fieldnames=fields)
    writer.writeheader()
    for (workload, keys), samples in groups.items():
        writer.writerow(dict(workload=workload, keys=keys, operations=samples[0]['operations'],
            median_seconds=statistics.median(float(r['seconds']) for r in samples),
            median_ns_per_operation=statistics.median(float(r['ns_per_operation']) for r in samples),
            capacity=samples[0]['capacity']))
metadata = dict(platform=platform.platform(), machine=platform.machine(),
    compiler=subprocess.check_output(['gcc', '--version'], text=True).splitlines()[0],
    flags='-std=c99 -O2 -Wall -Wextra -Wno-unused-parameter',
    trials=5, timer='C clock(): process CPU time',
    note='Synthetic single-thread benchmarks. Query selection and checksum work are timed. '
         'Lookup setup is excluded. Creation/insertion includes allocation and growth. '
         'Tiny insertion timings may be close to timer resolution. No results from the user PC.')
if Path('/proc/cpuinfo').exists():
    metadata['cpu'] = next((x.split(':', 1)[1].strip() for x in Path('/proc/cpuinfo').read_text().splitlines() if x.startswith('model name')), 'unknown')
(result_dir / 'environment.json').write_text(json.dumps(metadata, indent=2) + '\n')
print('Saved five trials and medians in benchmark_results/.')
