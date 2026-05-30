import subprocess
import os
import sys

SHELL_BINARY = "./myshell"

# We define the clean, expected outputs WITHOUT the shell prompt fluff
tests = [
    {
        "name": "Builtin: echo simple",
        "input": "echo hello world\n",
        "expected_lines": ["hello world"],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: echo with single quotes",
        "input": "echo 'hello    world'\n",
        "expected_lines": ["hello    world"],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: echo with double quotes",
        "input": "echo \"foo    bar\"\n",
        "expected_lines": ["foo    bar"],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: type for builtins",
        "input": "type echo\ntype exit\n",
        "expected_lines": ["echo is a shell builtin", "exit is a shell builtin"],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: type for external command",
        "input": "type ls\n",
        "expected_lines": ["ls is "],  # Soft match: just ensures it resolves the path
        "expected_stderr": ""
    },
    {
        "name": "Builtin: pwd",
        "input": "pwd\n",
        "expected_lines": [os.getcwd()],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: cd navigation",
        "input": "cd /\npwd\n",
        "expected_lines": ["/"],
        "expected_stderr": ""
    },
    {
        "name": "Parser: Unterminated Single Quote Error",
        "input": "echo 'broken quote\n",
        "expected_lines": [], 
        "expected_stderr": "syntax error: unterminated quote\n"
    },
    {
        "name": "Parser: Backslash Escape",
        "input": "echo hello\\ world\n",
        "expected_lines": ["hello world"],
        "expected_stderr": ""
    },
    {
        "name": "System: External Command Execution",
        "input": "whoami\n",
        "expected_lines": [os.getlogin() if hasattr(os, 'getlogin') else os.environ.get('USER', '')],
        "expected_stderr": ""
    }
]

def run_tests():
    if not os.path.exists(SHELL_BINARY):
        print(f"❌ Error: '{SHELL_BINARY}' not found. Compile your code first!")
        sys.exit(1)

    passed = 0
    failed = 0

    print("🚀 Starting Smart Shell Regression Tests...\n" + "="*50)

    for test in tests:
        process = subprocess.Popen(
            [SHELL_BINARY],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        try:
            stdout, stderr = process.communicate(input=test["input"], timeout=2)
            
            # Clean up stdout: remove prompt markers, exit text, and trailing empty space
            clean_stdout = stdout.replace("$ ", "").replace("Exiting Shell...", "").strip()
            actual_lines = [line.strip() for line in clean_stdout.split("\n") if line.strip()]

            # Validate lines
            stdout_match = True
            for expected in test["expected_lines"]:
                # Check if any of the actual output lines contain or match the expected string
                if not any(expected in actual for actual in actual_lines):
                    stdout_match = False
                    break
            
            stderr_match = stderr == test["expected_stderr"]

            if stdout_match and stderr_match:
                print(f"✅ PASSED: {test['name']}")
                passed += 1
            else:
                print(f"❌ FAILED: {test['name']}")
                print(f"   Input sent:      {repr(test['input'])}")
                if not stdout_match:
                    print(f"   Expected lines:  {test['expected_lines']}")
                    print(f"   Cleaned stdout:  {actual_lines}")
                if not stderr_match:
                    print(f"   Expected stderr: {repr(test['expected_stderr'])}")
                    print(f"   Got stderr:      {repr(stderr)}")
                failed += 1

        except subprocess.TimeoutExpired:
            print(f"⏰ TIMEOUT: {test['name']}")
            process.kill()
            failed += 1
        print("-" * 50)

    print(f"\n📊 Summary: {passed} passed, {failed} failed.")
    if failed > 0:
        sys.exit(1)

if __name__ == "__main__":
    run_tests()
