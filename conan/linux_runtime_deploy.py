import os
import shutil
import re

def _get_files_from_folder(search_dir, output_folder):
    if not os.path.isdir(search_dir):
        return []
    files = []
    for f in os.listdir(search_dir):
        src = os.path.join(search_dir, f)
        if os.access(src, os.X_OK) or bool(re.search(r"lib.*\.so(\.\d+)*$", f)):
            if os.path.isfile(src):
                dst = os.path.join(output_folder, f)
                shutil.copy2(src, dst)
                files.append(dst)
            else:
                files += _get_files_from_folder(src, output_folder)
    return files

def deploy(graph, output_folder, **kwargs):
    conanfile = graph.root.conanfile
    files = []
    for r, d in conanfile.dependencies.items():
        if d.package_folder is None:
            continue
        search_dirs = (d.cpp_info.libdirs or [])
        for dir in search_dirs:
            search_dir = os.path.join(d.package_folder, dir)
            files += _get_files_from_folder(search_dir, output_folder)
