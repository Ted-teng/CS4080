# ch 24 challenge 1 (compare only the instruction-pointer storage strategy)
import csv
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SUFFIX = '.exe' if os.name == 'nt' else ''
PROGRAMS = {'loop': 30000000, 'calls': 5702887, 'native_calls': 20000000}
VARIANTS = {'frame_ip': 'clox_baseline', 'local_ip': 'clox'}


def measure(variant, program):
    result = subprocess.run(
        [str(ROOT / (VARIANTS[variant] + SUFFIX)),
         str(ROOT / 'benchmarks' / (program + '.lox'))],
        capture_output=True, text=True, check=True, timeout=120)
    lines = result.stdout.strip().splitlines()
    valid = len(lines) == 2 and (lines[0] == 'true' if program == 'calls'
                                else float(lines[0]) == PROGRAMS[program])
    if not valid or result.stderr:
        raise RuntimeError('Unexpected benchmark output: ' + repr(result))
    seconds = float(lines[1])
    if seconds <= 0:
        raise RuntimeError('Timer resolution too low for this workload.')
    return seconds


subprocess.run(['make', 'clox', 'clox_baseline'], cwd=ROOT, check=True)
# Pin only this benchmark process and its children, if the OS permits it.
affinity = 'not available'
if hasattr(os, 'sched_getaffinity'):
    try:
        cpu = min(os.sched_getaffinity(0))
        os.sched_setaffinity(0, {cpu})
        affinity = [cpu]
    except OSError as error:
        affinity = str(error)
# One untimed warm-up process per program and variant.
for program in PROGRAMS:
    for variant in VARIANTS:
        measure(variant, program)
rows = []
for trial in range(1, 8):
    programs = list(PROGRAMS) if trial % 2 else list(reversed(PROGRAMS))
    variants = list(VARIANTS) if trial % 2 else list(reversed(VARIANTS))
    for program in programs:
        for variant in variants:
            rows.append({'trial': trial, 'program': program, 'variant': variant,
                         'seconds': measure(variant, program)})
results = ROOT / 'benchmark_results'
results.mkdir(exist_ok=True)
with (results / 'raw.csv').open('w', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=['trial', 'program', 'variant', 'seconds'])
    writer.writeheader()
    writer.writerows(rows)
summary = []
for program in PROGRAMS:
    medians = {variant: statistics.median(r['seconds'] for r in rows
              if r['program'] == program and r['variant'] == variant)
              for variant in VARIANTS}
    baseline = medians['frame_ip']
    optimized = medians['local_ip']
    summary.append({'program': program, 'frame_ip_seconds': baseline,
                    'local_ip_seconds': optimized,
                    'time_reduction_percent': 100 * (baseline - optimized) / baseline})
with (results / 'medians.csv').open('w', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=list(summary[0]))
    writer.writeheader()
    writer.writerows(summary)
metadata = {
    'platform': platform.platform(),
    'compiler': subprocess.check_output(['gcc', '--version'], text=True).splitlines()[0],
    'build_command': 'make clox clox_baseline (default Makefile flags)',
    'flags': '-std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -lm',
    'difference': 'clox_baseline defines CLOX_BASELINE_IP; native arity/error support is identical.',
    'timer': 'Lox clock(), backed by C clock(): process CPU time',
    'trials': 7,
    'cpu_affinity': affinity,
    'timed_region': 'workload execution; excludes source reading, compilation, and printing',
    'limitations': 'Synthetic programs in a shared environment; no register-allocation proof. '
                   'Native-call results do not isolate the cost of arity or error checking.'
}
if Path('/proc/cpuinfo').exists():
    metadata['cpu'] = next((line.split(':', 1)[1].strip()
        for line in Path('/proc/cpuinfo').read_text().splitlines()
        if line.startswith('model name')), 'unknown')
(results / 'environment.json').write_text(json.dumps(metadata, indent=2) + '\n')
for row in summary:
    print('{program}: frame={frame_ip_seconds:.6f}s local={local_ip_seconds:.6f}s '
          'time reduction={time_reduction_percent:.1f}%'.format(**row))
