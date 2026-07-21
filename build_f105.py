import rlimit, subprocess, sys, os
rlimit.apply()

build_dir = r'E:\STM32project\rm_2027\mas_embedded_threadx-f103\build\f105_rc\Debug'
board_dir = r'E:\STM32project\rm_2027\mas_embedded_threadx-f103\board\f105_rc'
os.makedirs(build_dir, exist_ok=True)

# CMake configure
result = subprocess.run(
    ['cmake', '-S', board_dir, '-B', build_dir, '--preset', 'Debug', '-G', 'Ninja'],
    capture_output=True, text=True
)
print('=== CMAKE CONFIGURE ===')
print(result.stdout[-2000:])
if result.stderr:
    print(result.stderr[-2000:])
if result.returncode != 0:
    print(f'CMAKE FAILED: {result.returncode}')
    sys.exit(1)

# CMake build
result = subprocess.run(
    ['cmake', '--build', build_dir, '--config', 'Debug', '-j4'],
    capture_output=True, text=True
)
print('=== CMAKE BUILD ===')
out = result.stdout
err = result.stderr
print(out[-3000:])
if err:
    print(err[-3000:])
print(f'BUILD EXIT CODE: {result.returncode}')
