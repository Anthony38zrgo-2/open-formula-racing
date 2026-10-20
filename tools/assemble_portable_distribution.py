import argparse
import collections
import hashlib
import json
import pathlib
import re
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--project', required=True)
    parser.add_argument('--compiler-prefix', required=True)
    arguments = parser.parse_args()
    project = pathlib.Path(arguments.project)
    compiler_prefix = pathlib.Path(arguments.compiler_prefix)
    distribution = project / 'distribution'
    data_directory = distribution / 'share/games/speed-dreams-2'
    binaries_directory = distribution / 'bin'
    shutil.copytree(project / 'implementation/candidates/fuji-seventy-six-seventy-seven', data_directory / 'tracks/circuit/fuji-seventy-six-seventy-seven', dirs_exist_ok=True)
    shutil.copytree(project / 'implementation/candidates/williams-nineteen-ninety-seven', data_directory / 'cars/models/williams-nineteen-ninety-seven', dirs_exist_ok=True)
    shutil.copytree(project / 'implementation/initial-settings', distribution / 'initial-settings', dirs_exist_ok=True)
    shutil.copytree(project / 'implementation/initial-settings/cars/categories', data_directory / 'cars/categories', dirs_exist_ok=True)
    shutil.copyfile(project / 'implementation/initial-settings/config/raceman/williams-fuji-practice.xml', data_directory / 'config/raceman/williams-fuji-practice.xml')
    plugin_directory = compiler_prefix / 'bin/osgPlugins-3.6.5'
    if not plugin_directory.exists():
        plugin_directory = compiler_prefix / 'lib/osgPlugins-3.6.5'
    destination_plugins = binaries_directory / 'osgPlugins-3.6.5'
    destination_plugins.mkdir(parents=True, exist_ok=True)
    for plugin_name in ['osgdb_png.dll', 'osgdb_jpeg.dll', 'osgdb_rgb.dll', 'osgdb_freetype.dll', 'osgdb_osg.dll', 'osgdb_serializers_osg.dll', 'osgdb_glsl.dll', 'osgdb_ac.dll']:
        matching_plugins = list(plugin_directory.glob('*' + plugin_name))
        if len(matching_plugins) != 1:
            raise RuntimeError('Expected exactly one native plugin: ' + plugin_name)
        source_plugin = matching_plugins[0]
        shutil.copyfile(source_plugin, destination_plugins / source_plugin.name)
    inspection_program = compiler_prefix / 'bin/objdump.exe'
    pending = collections.deque(list(distribution.rglob('*.dll')) + list(distribution.rglob('*.exe')))
    inspected = set()
    windows_directories = [pathlib.Path('C:/Windows/System32'), pathlib.Path('C:/Windows')]
    unresolved = set()
    while pending:
        binary = pending.popleft()
        if str(binary) in inspected:
            continue
        inspected.add(str(binary))
        inspection = subprocess.run([str(inspection_program), '-p', str(binary)], capture_output=True, text=True, check=True).stdout
        for library_name in re.findall(r'DLL Name:\s*([^\r\n]+)', inspection):
            library_name = library_name.strip()
            if library_name.lower().startswith(('api-ms-', 'ext-ms-')) or any((directory / library_name).exists() for directory in windows_directories):
                continue
            destination = binaries_directory / library_name
            if not destination.exists():
                source = compiler_prefix / 'bin' / library_name
                if not source.exists():
                    unresolved.add(library_name)
                    continue
                shutil.copyfile(source, destination)
            pending.append(destination)
    if unresolved:
        raise RuntimeError('Unresolved portable dependencies: ' + ', '.join(sorted(unresolved)))
    manifest = {'runtime_dependencies_resolved': True, 'files': []}
    for path in sorted(distribution.rglob('*')):
        if path.is_file() and 'user-settings' not in path.parts:
            manifest['files'].append({'path': path.relative_to(distribution).as_posix(), 'size': path.stat().st_size,
                                      'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    (distribution / 'distribution_manifest.json').write_text(json.dumps(manifest, indent=2))
    print(json.dumps({'files': len(manifest['files']), 'inspected_binaries': len(inspected), 'unresolved_dependencies': sorted(unresolved)}))


if __name__ == '__main__':
    main()
