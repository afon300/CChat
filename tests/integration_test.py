import subprocess
import time
import os
import sys

def run_test():
    print("Starting integration test...")
    
    # 1. Start server
    server_path = os.path.join("bin", "server")
    if sys.platform == "win32":
        server_path += ".exe"
    
    if not os.path.exists(server_path):
        print(f"Error: Server binary not found at {server_path}. Run 'make' first.")
        return False

    # Start server
    server_process = subprocess.Popen([server_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    time.sleep(1) # Wait for server to start

    # 2. Start client
    client_path = os.path.join("bin", "main")
    if sys.platform == "win32":
        client_path += ".exe"

    if not os.path.exists(client_path):
        print(f"Error: Client binary not found at {client_path}. Run 'make' first.")
        server_process.terminate()
        return False

    # Simulate user input: Server IP, Nickname, Message, /exit
    inputs = "127.0.0.1\nTester\nHello World\n/exit\n"
    
    try:
        # Run client and pipe inputs
        client_process = subprocess.run([client_path], input=inputs, capture_output=True, text=True, timeout=10)
        print("Client finished.")
        
        # Check client output
        if "you : Hello World" in client_process.stdout:
            print("Test PASSED: Client sent and received message locally.")
        else:
            print("Test FAILED: Message not found in client output.")
            # print("Output was:", client_process.stdout) # Debug
            server_process.terminate()
            return False

    except subprocess.TimeoutExpired:
        print("Test FAILED: Client timed out.")
        server_process.terminate()
        return False
    except Exception as e:
        print(f"Test FAILED: {e}")
        server_process.terminate()
        return False
    finally:
        server_process.terminate()
        try:
            server_process.wait(timeout=2)
        except:
            server_process.kill()

    return True

if __name__ == "__main__":
    if run_test():
        sys.exit(0)
    else:
        sys.exit(1)
