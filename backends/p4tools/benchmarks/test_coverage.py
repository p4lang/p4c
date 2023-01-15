import os
import subprocess
import random
import csv

# generate random seeds, increase the number for extra sampling
ITERATIONS = 2
seeds = []
for s in range(ITERATIONS):
	seed = random.randint(1001, 10001)
	seeds.append(seed)

# testgen bin location, i.e. /home/p4/p4c/build/p4testgen
p4testgen = "/p4/p4c/build/p4testgen"

# initial max tests to be measured
max_tests = 10

# target program location
p4_program = "/p4/p4c/testdata/p4_16_samples/pins/pins_middleblock.p4"

p4_program_name = p4_program.split("/")[-1].split(".")[0]

out_dir = f"/p4/p4c/build/testgen/testgen-p4c-bmv2/{p4_program_name}.out"

includes = "/p4/p4c/build/p4include"
extras = ""

# csv results file path
results_path = f"/results_{p4_program_name}.csv"
header = ["seed", "max_tests_input", "DFS Coverage", "max_tests_generated", "time (ms)", "Random Access Stack",
"max_tests_generated", 
"time (ms)", "Random Access Max Cov", "max_tests_generated", "time (ms)"]

def run_strategies_for_max_tests():
	data_row = [seed, max_tests]

	# DFS
	proc = subprocess.run(
	    [p4testgen, "--target", "bmv2", "--arch", "v1model", "--std", "p4-16", "-I", 
	    includes, "--test-backend", "STF", "--strict", "--print-traces", "--print-performance-report", 
	    "--seed", str(seed), "--max-tests", str(max_tests), "--out-dir", out_dir, p4_program],
	    stdout=subprocess.PIPE,
	    stderr=subprocess.PIPE,
	    text=True
	)
	
	max_test_generated = str(proc.stderr).split("End Test ")[-1].split("=")[0].strip()
	
	split_str = f"Test {max_test_generated}: Statements covered: "
	statements_cov = str(proc.stderr).split(split_str)[-1].split("(")[0]
	print(statements_cov)
	data_row.append(statements_cov)
	data_row.append(max_test_generated)

	time = str(proc.stderr).split("Timers")[-1].split("Total: ")[-1].split("ms")[0]
	data_row.append(time)

	# random access stack; pop-level fixed at 3

	proc = subprocess.run(
	    [p4testgen, "--target", "bmv2", "--arch", "v1model", "--std", "p4-16", "-I", 
	    includes, "--test-backend", "STF", "--strict", "--print-traces", "--print-performance-report", 
	    "--seed", str(seed), "--max-tests", str(max_tests), "--out-dir", out_dir, p4_program, 
	    "--exploration-strategy", "RANDOM_ACCESS_STACK", "--pop-level", "3"],
	    stdout=subprocess.PIPE,
	    stderr=subprocess.PIPE,
	    text=True
	)
	
	max_test_generated = str(proc.stderr).split("End Test ")[-1].split("=")[0].strip()
	
	split_str = f"Test {max_test_generated}: Statements covered: "
	statements_cov = str(proc.stderr).split(split_str)[-1].split("(")[0]
	print(statements_cov)
	data_row.append(statements_cov)
	data_row.append(max_test_generated)

	time = str(proc.stderr).split("Timers")[-1].split("Total: ")[-1].split("ms")[0]
	data_row.append(time)
	
	# random access max cov, saddle-point fixed at 2
	proc = subprocess.run(
	    [p4testgen, "--target", "bmv2", "--arch", "v1model", "--std", "p4-16", "-I", 
	    includes, "--test-backend", "STF", "--strict", "--print-traces", "--print-performance-report", 
	    "--seed", str(seed), "--max-tests", str(max_tests), "--out-dir", out_dir, p4_program, 
	    "--exploration-strategy", "RANDOM_ACCESS_MAX_COVERAGE", "--saddle-point", "2"],
	    stdout=subprocess.PIPE,
	    stderr=subprocess.PIPE,
	    text=True
	)
	
	max_test_generated = str(proc.stderr).split("End Test ")[-1].split("=")[0].strip()
	
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
	writer = csv.writer(f)
	writer.writerow(header)
	for seed in seeds:
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
