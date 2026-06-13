import subprocess
import os
import sys
import getpass
import platform

SHELL_BINARY = "./shell"

is_linux = platform.system() == "Linux"

linux_expected = "ls: cannot access 'nonexistent_file': No such file or directory\nls: cannot access 'another_nonexistent_file': No such file or directory\n"
mac_expected = "ls: nonexistent_file: No such file or directory\nls: another_nonexistent_file: No such file or directory\n"

tests = [
    {
        "name": "Builtin: print simple",
        "input": "print hello world\n",
        "expected_lines": ["hello world"],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: print with single quotes",
        "input": "print 'hello    world'\n",
        "expected_lines": ["hello    world"],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: print with double quotes",
        "input": "print \"foo    bar\"\n",
        "expected_lines": ["foo    bar"],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: whatis for builtins",
        "input": "whatis print\nwhatis exit\nwhatis pwd\nwhatis go\nwhatis whatis\n",
        "expected_lines": [
            "print is a shell builtin",
            "exit is a shell builtin",
            "pwd is a shell builtin",
            "go is a shell builtin",
            "whatis is a shell builtin"
        ],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: whatis for external command",
        "input": "whatis ls\n",
        "expected_lines": ["ls is "],  # Soft match: just ensures it resolves the path
        "expected_stderr": ""
    },
    {
        "name": "Builtin: whatis for unknown command",
        "input": "whatis fakecommand\n",
        "expected_lines": ["fakecommand: not found"],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: pwd",
        "input": "pwd\n",
        "expected_lines": [os.getcwd()],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: go navigation",
        "input": "go /\npwd\n",
        "expected_lines": ["/"],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: go home with tilde",
        "input": "go ~\npwd\n",
        "expected_lines": [os.path.expanduser("~")],
        "expected_stderr": ""
    },
    {
        "name": "Builtin: go invalid path",
        "input": "go /nonexistent_dir_xyz\n",
        "expected_lines": [],
        "expected_stderr": "go: /nonexistent_dir_xyz: No such file or directory\n"
    },
    {
        "name": "Parser: Unterminated Single Quote Error",
        "input": "print 'broken quote\n",
        "expected_lines": [],
        "expected_stderr": "syntax error: unterminated quote\n"
    },
    {
        "name": "Parser: Unterminated Double Quote Error",
        "input": "print \"broken quote\n",
        "expected_lines": [],
        "expected_stderr": "syntax error: unterminated quote\n"
    },
    {
        "name": "Parser: Backslash Escape",
        "input": "print hello\\ world\n",
        "expected_lines": ["hello world"],
        "expected_stderr": ""
    },
    {
        "name": "Parser: Empty input",
        "input": "\n",
        "expected_lines": [],
        "expected_stderr": ""
    },
    {
        "name": "System: External Command Execution",
        "input": "whoami\n",
        "expected_lines": [getpass.getuser()],
        "expected_stderr": ""
    },
    {
        "name": "System: Unknown command error",
        "input": "fakecommand_xyz\n",
        "expected_lines": ["fakecommand_xyz: command not found"],
        "expected_stderr": ""
    },

    # --- STDOUT REDIRECTION TESTS (> and 1>) ---
    {
        "name": "Redirection: Builtin print to file",
        "input": "print hello redirection > test_print.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_print.txt",
            "content": "hello redirection\n"
        }
    },
    {
        "name": "Redirection: Builtin pwd to file",
        "input": "pwd > test_pwd.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_pwd.txt",
            "content": os.getcwd() + "\n"
        }
    },
    {
        "name": "Redirection: External program output to file",
        "input": "whoami > test_ext.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_ext.txt",
            "content": getpass.getuser() + "\n"
        }
    },
    {
        "name": "Redirection: Explicit stdout (1>) with print",
        "input": "print hello explicit stdout 1> test_explicit_print.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_explicit_print.txt",
            "content": "hello explicit stdout\n"
        }
    },
    {
        "name": "Redirection: Explicit stdout (1>) with external command",
        "input": "whoami 1> test_explicit_ext.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_explicit_ext.txt",
            "content": getpass.getuser() + "\n"
        }
    },
    {
        # Verifies that only the first redirection token is consumed;
        # remaining tokens are passed as print arguments
        "name": "Redirection: Greedy token trailing argument edge case",
        "input": "print hello world > file.txt t.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "file.txt",
            "content": "hello world t.txt\n"
        }
    },

    # --- STDERR REDIRECTION TESTS (2>) ---
    {
        "name": "Redirection: stderr (2>) to file on bad go",
        "input": "go /nonexistent_dir_xyz 2> test_stderr.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_stderr.txt",
            "content": "go: /nonexistent_dir_xyz: No such file or directory\n"
        }
    },
    {
        "name": "Redirection: stderr (2>) to file on unknown command",
        "input": "fakecommand_xyz 2> test_stderr_cmd.txt\n",
        "expected_lines": ["fakecommand_xyz: command not found"],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_stderr_cmd.txt",
            "content": ""
        }
    },
    {
        "name": "Redirection: Append short (>>)",
        "input": "print first line > test_append.txt\nprint second line >> test_append.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_append.txt",
            "content": "first line\nsecond line\n"
        }
    },
    {
        "name": "Redirection: Append explicit stdout (1>>)",
        "input": "print stream one 1> test_append_explicit.txt\nprint stream two 1>> test_append_explicit.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_append_explicit.txt",
            "content": "stream one\nstream two\n"
        }
    },
    {
        "name": "Redirection: Append explicit stderr (2>>)",
        "input": "ls nonexistent_file 2>> test_append_explicit_stderr.txt\nls another_nonexistent_file 2>> test_append_explicit_stderr.txt\n",
        "expected_lines": [],
        "expected_stderr": "",
        "expected_file": {
            "path": "test_append_explicit_stderr.txt",
            "content": linux_expected if is_linux else mac_expected
        }
    },
    {
        "name": "Builtin: complete -p outputs error for unknown spec",
        "input": "complete -p mycmd\n",
        "expected_lines": [],
        "expected_stderr": "complete: mycmd: can't find the completion specification.\n"
    },
    {
        "name": "Builtin: complete with wrong argument count",
        "input": "complete -p\n",
        "expected_lines": [],
        "expected_stderr": "error: complete should have two arguments.\n"
    },
]


def run_tests():
    if not os.path.exists(SHELL_BINARY):
        print(f"❌ Error: '{SHELL_BINARY}' not found. Compile your code first!")
        sys.exit(1)

    passed = 0
    failed = 0

    print("🚀 Starting Smart Shell Regression Tests...\n" + "=" * 50)

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

            # Validate stdout lines
            stdout_match = True
            for expected in test["expected_lines"]:
                if not any(expected in actual for actual in actual_lines):
                    stdout_match = False
                    break

            # Validate stderr
            stderr_match = stderr == test["expected_stderr"]

            # Validate generated side-effect files (Redirection check)
            file_match = True
            file_error_msg = ""

            if "expected_file" in test:
                file_info = test["expected_file"]
                file_path = file_info["path"]
                expected_content = file_info["content"]

                if expected_content == "" and not os.path.exists(file_path):
                    # Empty content + no file is acceptable
                    pass
                elif not os.path.exists(file_path):
                    file_match = False
                    file_error_msg = f"Expected file '{file_path}' was never created."
                else:
                    with open(file_path, "r") as f:
                        actual_content = f.read()

                    if actual_content != expected_content:
                        file_match = False
                        file_error_msg = (
                            f"File content mismatch.\n"
                            f"     Expected: {repr(expected_content)}\n"
                            f"     Got:      {repr(actual_content)}"
                        )

                    os.remove(file_path)

            # Final Verdict
            if stdout_match and stderr_match and file_match:
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
                if not file_match:
                    print(f"   File Error:      {file_error_msg}")
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
