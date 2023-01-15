import os
import subprocess
import random
import csv
import dateutil
import pandas as pd
import argparse

# generate random seeds, increase the number for extra sampling
ITERATIONS = 1
# 7189 is an example of a good seed, which gets cov 1 with less than 100 tests
# in random access stack.
seeds = [7189]
for s in range(ITERATIONS):
	seed = random.randint(1001, 10001)
	seeds.append(seed)

# testgen bin location, i.e. /home/p4/p4c/build/p4testgen
p4testgen = "/p4/p4c/build/p4testgen"

# initial max tests to be measured


# target program location
p4_program = "/p4/p4c/testdata/p4_16_samples/pins/pins_middleblock.p4"

p4_program_name = p4_program.split("/")[-1].split(".")[0]

out_dir = f"/p4/p4c/build/testgen/testgen-p4c-bmv2/{p4_program_name}.out"

includes = "/p4/p4c/build/p4include"

parser = argparse.ArgumentParser()
parser.add_argument('--exhaust', action='store_true')

extras = ""
extra_val = ""

# csv results file path
results_path = f"/results_{p4_program_name}.csv"
results_path_max = f"/max_results_{p4_program_name}.csv"
results_path_exhaust = f"/exhaust_results_{p4_program_name}.csv"

header = ["seed", "max_tests_input", "DFS Coverage", "max_cov_on_test", "time (ms)", "Random Access Stack",
"max_cov_on_test", 
"time (ms)", "Random Access Max Cov", "max_cov_on_test", "time (ms)"]


args = parser.parse_args()

def run_strategies_for_max_tests():
	data_row = [seed, max_tests]
	commands = [p4testgen, "--target", "bmv2", "--arch", "v1model", "--std", "p4-16", "-I", 
	    includes, "--test-backend", "STF", "--strict", "--print-traces", "--print-performance-report", 
	    "--seed", str(seed), "--max-tests", str(max_tests), "--out-dir", out_dir, p4_program, extras, extra_val]
	commands = [c for c in commands if c]
	# DFS
	proc = subprocess.run(
	    commands,
	    stdout=subprocess.PIPE,
	    stderr=subprocess.PIPE,
	    text=True
	)
	
	max_test_generated_arr = str(proc.stderr).split(": Statements covered: 1")
	max_test_generated = max_test_generated_arr[0].split("Test")[-1].strip() if (len(max_test_generated_arr) > 1) else str(proc.stderr).split("End Test ")[-1].split("=")[0].strip()
	
	split_str = f"Test {max_test_generated}: Statements covered: "
	statements_cov = str(proc.stderr).split(split_str)[-1].split("(")[0]
	print(statements_cov)
	data_row.append(statements_cov)
	data_row.append(max_test_generated)

	time = str(proc.stderr).split("Timers")[-1].split("Total: ")[-1].split("ms")[0]
	data_row.append(time)

	# random access stack; pop-level fixed at 3
	commands = [p4testgen, "--target", "bmv2", "--arch", "v1model", "--std", "p4-16", "-I", 
	    includes, "--test-backend", "STF", "--strict", "--print-traces", "--print-performance-report", 
	    "--seed", str(seed), "--max-tests", str(max_tests), "--out-dir", out_dir, p4_program, 
	    "--exploration-strategy", "RANDOM_ACCESS_STACK", "--pop-level", "3", extras, extra_val]
	commands = [c for c in commands if c]
	proc = subprocess.run(
	    commands,
	    stdout=subprocess.PIPE,
	    stderr=subprocess.PIPE,
	    text=True
	)
	
	max_test_generated_arr = str(proc.stderr).split(": Statements covered: 1")
	max_test_generated = max_test_generated_arr[0].split("Test")[-1].strip() if (len(max_test_generated_arr) > 1) else str(proc.stderr).split("End Test ")[-1].split("=")[0].strip()
		
	split_str = f"Test {max_test_generated}: Statements covered: "
	statements_cov = str(proc.stderr).split(split_str)[-1].split("(")[0]
	print(statements_cov)
	data_row.append(statements_cov)
	data_row.append(max_test_generated)

	time = str(proc.stderr).split("Timers")[-1].split("Total: ")[-1].split("ms")[0]
	data_row.append(time)
	
	# random access max cov, saddle-point fixed at 2
	commands = [p4testgen, "--target", "bmv2", "--arch", "v1model", "--std", "p4-16", "-I", 
	    includes, "--test-backend", "STF", "--strict", "--print-traces", "--print-performance-report", 
	    "--seed", str(seed), "--max-tests", str(max_tests), "--out-dir", out_dir, p4_program, 
	    "--exploration-strategy", "RANDOM_ACCESS_MAX_COVERAGE", "--saddle-point", "3"]
	commands = [c for c in commands if c]

	proc = subprocess.run(
	    commands,
	    stdout=subprocess.PIPE,
	    stderr=subprocess.PIPE,
	    text=True
	)
	
	max_test_generated_arr = str(proc.stderr).split(": Statements covered: 1")
	max_test_generated = max_test_generated_arr[0].split("Test")[-1].strip() if (len(max_test_generated_arr) > 1) else str(proc.stderr).split("End Test ")[-1].split("=")[0].strip()
		
	split_str = f"Test {max_test_generated}: Statements covered: "
	statements_cov = str(proc.stderr).split(split_str)[-1].split("(")[0]
	# print(proc.stderr)
	print(statements_cov)
	data_row.append(statements_cov)
	data_row.append(max_test_generated)
	time = str(proc.stderr).split("Timers")[-1].split("Total: ")[-1].split("ms")[0]
	data_row.append(time)
	return data_row
	

with open(results_path, 'w') as f:
	print("Generating basic metrics up to 100 tests")
	writer = csv.writer(f)
	writer.writerow(header)
	for seed in seeds:
		max_tests = 10
		data_row = run_strategies_for_max_tests()
		writer.writerow(data_row)
		
		max_tests = max_tests + 10
		data_row = run_strategies_for_max_tests()
		writer.writerow(data_row)

		max_tests = max_tests + 10
		data_row = run_strategies_for_max_tests()
		writer.writerow(data_row)

		max_tests = 100
		data_row = run_strategies_for_max_tests()
		writer.writerow(data_row)

with open(results_path_max, 'w') as f:
	print("Generating metrics up to 1000 tests")
	writer = csv.writer(f)
	writer.writerow(header)
	for seed in seeds:
		max_tests = 1000
		data_row = run_strategies_for_max_tests()
		writer.writerow(data_row)

if args.exhaust:
	print("Exhaust")
	with open(results_path_exhaust, 'w') as f:
		writer = csv.writer(f)
		writer.writerow(header)
		for seed in seeds:
			max_tests = 0
			extras = "--stop-metric"
			extra_val = "MAX_STATEMENT_COVERAGE"
			data_row = run_strategies_for_max_tests()
			writer.writerow(data_row)