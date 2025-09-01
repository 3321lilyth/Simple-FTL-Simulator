import sys

if len(sys.argv) < 2:
    print("Usage: python script.py <param1> <param2> ... <paramN>")
    sys.exit(1)

params = sys.argv[1:]

print("Received parameters:", params)
