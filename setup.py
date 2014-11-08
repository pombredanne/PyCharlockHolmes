from distutils.core import setup, Extension
import os

#os.environ["CC"] = "clang"
#os.environ["CXX"] = "clang++"

# Use environment variable ICUI18N to set your own icui18n configuration.
icui18n_path = os.getenv("ICUI18N")
if icui18n_path is None:
        icui18n_path = '/usr/local'

icui18n_bin = os.path.join(icui18n_path, 'bin')
icui18n_include = os.path.join(icui18n_path, 'include')
icui18n_lib = os.getenv('ICUI18N_LIB', os.path.join(icui18n_path, 'lib'))

magic_path = os.getenv("MAGIC")
if magic_path is None:
        magic_path = '/usr/local'
magic_bin = os.path.join(magic_path, 'bin')
magic_include = os.path.join(magic_path, 'include')
magic_lib = os.getenv('MAGIC_LIB', os.path.join(magic_path, 'lib'))

# os.system('cd ./lib; ./build.sh')

ch_exts = [
    os.path.join('src', name) for name in os.listdir('src')
    if name.endswith('.c')
]

# ch_module = Extension('charlockholmes', ch_exts, include_dirs=['./lib/magic/include'], library_dirs=['./lib/magic/lib'], libraries=['icui18n'])
ch_module = Extension('pycharlockholmes',
                      ch_exts,
                      include_dirs=[icui18n_include, magic_include],
                      library_dirs=[icui18n_lib, magic_lib],
                      libraries=['icui18n', 'magic'])

setup (
    name='pycharlockholmes',
    version='0.0.4',
    description='Character encoding detecting library for Python using '
                'ICU and libmagic. Based on Ruby '
                'implementation https://github.com/brianmario/charlock_holmes '
                'and work of https://github.com/xtao/PyCharlockHolmes',
    url='https://github.com/kkszysiu/pycharlockholmes',
    ext_modules=[ch_module],
    keywords=('icu', 'magic', 'charlockholmes', 'egg', 'mime', 'libmagic'),
    license='Modified BSD License',
    author='Krzysztof "kkszysiu" Klinikowski',
    author_email='kkszysiu@gmail.com',
)
