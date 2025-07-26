import os
import sys
ver=""
if len(sys.argv) > 2:
    print('Usage: install / install python.ver')
    print('Example: install 3.10.6')
    print('Example: install')
    sys.exit(1)
elif len(sys.argv) == 2:
    ver = sys.argv[1]
os.system("pip "+ver+" install -r requirements.txt")