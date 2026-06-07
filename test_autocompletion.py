import os
import pty
import sys
import time
import subprocess

SHELL_BINARY = "./shell"


def test_autocomplete():
    print("🧪 Testing Tab-Completion via Pseudo-Terminal (PTY)...")

    master_fd, slave_fd = pty.openpty()

    process = subprocess.Popen(
        [SHELL_BINARY],
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        text=True,
        close_fds=False
    )
    os.close(slave_fd)

    try:
        # Synchronize: Wait for raw mode initialization
        boot_buffer = ""
        start_time = time.time()
        while "$ " not in boot_buffer:
            if time.time() - start_time > 2.0:
                print("❌ TIMEOUT: Shell prompt '$ ' never appeared during boot.")
                print(f"Captured: {repr(boot_buffer)}")
                return

            try:
                boot_buffer += os.read(master_fd, 1).decode('utf-8')
            except OSError:
                break

        os.write(master_fd, b"ex\t\n")
        time.sleep(0.2)

        output = os.read(master_fd, 4096).decode('utf-8')

        if "exit" in output:
            print("✅ PASSED: 'e' successfully autocompleted to 'exit'!")
        else:
            print("❌ FAILED: Autocompletion did not trigger.")
            print(f"Captured Terminal Output:\n{repr(output)}")

    finally:
        os.close(master_fd)
        process.terminate()
        process.wait()


if __name__ == "__main__":
    if not os.path.exists(SHELL_BINARY):
        print(f"Error: Build '{SHELL_BINARY}' first.")
        sys.exit(1)

    test_autocomplete()
