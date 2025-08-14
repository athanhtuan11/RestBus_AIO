import shutil

def copy_dll_folder():
    """
    Copy toàn bộ thư mục templates_Canlyzer/dll sang Canlyzer_Output/dll
    """
    import os
    src = os.path.join(os.path.dirname(__file__), 'templates_Canlyzer', 'dll')
    dst = os.path.join(os.path.dirname(__file__), 'Canlyzer_Output', 'dll')
    if os.path.exists(dst):
        # Nếu thư mục đích đã tồn tại, xóa trước rồi copy lại
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    print(f"Đã copy thư mục dll sang: {dst}")
# Autogen module for RestBus AIO
import os
def run_autogen():
    """Main entry for AutoGen logic. Add your automation code here."""
    print("AutoGen: Running automation tasks...")
    # Example: You can call other helper functions here
    analyze_cfg()
    # ... add more automation steps as needed

def analyze_cfg():
    """Example function: Analyze MRR1plus.cfg and print DBC file types."""
    import os
    cfg_path = os.path.join(os.path.dirname(__file__), 'templates_Canlyzer', 'MRR1plus.cfg')
    if not os.path.isfile(cfg_path):
        print(f"File not found: {cfg_path}")
        return
    with open(cfg_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
    in_block = False
    for line in lines:
        if line.strip().startswith('VDatabaseContainerStreamer') and 'Begin_Of_Object' in line:
            in_block = True
            continue
        if in_block and line.strip().startswith('End_Of_Object VDatabaseContainerStreamer'):
            break
        if in_block and '<VFileName' in line and '.dbc' in line:
            fname = line.split('"')[-2]
            ftype = 'private' if 'pri' in fname.lower() or 'private' in fname.lower() else 'public'
            print(f"{fname} : {ftype}")

def replace_dbc_in_cfg(input_cfg, output_cfg, dbc_pub, dbc_pri, can_type='PUB'):
    """
    Tạo file cfg mới, thay thế DBC file cho public hoặc private dựa vào can_type (PUB/PRI)
    """

    with open(input_cfg, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
    in_block = False
    new_lines = []
    for line in lines:
        if line.strip().startswith('VDatabaseContainerStreamer') and 'Begin_Of_Object' in line:
            in_block = True
            new_lines.append(line)
            continue
        if in_block and line.strip().startswith('End_Of_Object VDatabaseContainerStreamer'):
            in_block = False
            new_lines.append(line)
            continue
        if in_block and '<VFileName' in line and '.dbc' in line:
            lower_line = line.lower()
            if 'pri' in lower_line or 'private' in lower_line:
                if can_type.upper() == 'PRI' and dbc_pri:
                    prefix = line.split('"')[0] + '"'
                    # Lấy đường dẫn tương đối bắt đầu từ 'dbc\'
                    rel_path = dbc_pri
                    if os.path.isabs(dbc_pri):
                        idx = dbc_pri.lower().find('dbc'+os.sep)
                        if idx != -1:
                            rel_path = dbc_pri[idx:]
                        else:
                            rel_path = os.path.basename(dbc_pri)
                    new_lines.append(f'{prefix}{rel_path}"\n')
                else:
                    new_lines.append(line)
            else:
                if can_type.upper() == 'PUB' and dbc_pub:
                    prefix = line.split('"')[0] + '"'
                    rel_path = dbc_pub
                    if os.path.isabs(dbc_pub):
                        idx = dbc_pub.lower().find('dbc'+os.sep)
                        if idx != -1:
                            rel_path = dbc_pub[idx:]
                        else:
                            rel_path = os.path.basename(dbc_pub)
                    new_lines.append(f'{prefix}{rel_path}"\n')
                else:
                    new_lines.append(line)
        else:
            new_lines.append(line)
    with open(output_cfg, 'w', encoding='utf-8') as f:
        f.writelines(new_lines)

def autogen_create_cfg(dbc_pub, dbc_pri, can_type='PUB'):
    """
    Hàm gọi từ GUI: tạo file MRR1plus.cfg mới với DBC phù hợp
    """
    input_cfg = os.path.join(os.path.dirname(__file__), 'templates_Canlyzer', 'MRR1plus.cfg')
    output_cfg = os.path.join(os.path.dirname(__file__), 'Canlyzer_Output', 'MRR1plus.cfg')
    replace_dbc_in_cfg(input_cfg, output_cfg, dbc_pub, dbc_pri, can_type)
    copy_dll_folder()
    print(f"Đã tạo file mới: {output_cfg}")
