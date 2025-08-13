import math
import re
import os
import tkinter as tk
from tkinter import filedialog, messagebox

# Biến toàn cục
WorkingPath = ''
DBCPath = ''
RestbusPath = ''
NodeName = ''
Channel = ''

# Hàm phụ trợ

def get_doc_path():
    return os.path.join(os.path.expanduser('~'), 'Documents')

def save_log():
    doc_path = get_doc_path()
    log_path = os.path.join(doc_path, 'dblog')
    with open(log_path, 'w', encoding='utf-8') as f:
        f.write(f"{WorkingFolder_var.get()}\n{DBC_var.get()}\n{Node_var.get()}\n{Channel_var.get()}\n{tb_RestbusPath_var.get()}\n")

def load_log():
    doc_path = get_doc_path()
    log_path = os.path.join(doc_path, 'dblog')
    if os.path.exists(log_path):
        with open(log_path, 'r', encoding='utf-8') as f:
            lines = f.read().splitlines()
            return lines + [''] * (5 - len(lines))
    return [''] * 5

def get_info():
    import pandas as pd
    global DBCPath, NodeName, WorkingPath
    DBCPath = DBC_var.get()
    NodeName = Node_var.get()
    path = WorkingFolder_var.get()
    if not path.endswith('\\'):
        path += '\\'
    WorkingPath = path

    # --- Xử lý file Excel tương tự GetInfo VBA nếu cần ---
    # Ví dụ: excel_path = filedialog.askopenfilename(title="Chọn file Excel", filetypes=[("Excel files", "*.xlsx;*.xls")])
    # Nếu bạn muốn tự động lấy file theo WorkingPath, có thể sửa lại dòng dưới:
    excel_path = os.path.join(WorkingPath, "input.xlsx")
    if not os.path.isfile(excel_path):
        # Tạo file Excel mẫu nếu chưa có
        import pandas as pd
        columns = ["Command", "Data", "Value", "Expected", "Comment", "TestcaseID"]
        # Dữ liệu mẫu, bạn có thể chỉnh lại cho phù hợp
        data = [
            ["Comments", "Heading1", "", "", "REQ001", "TC001"],
            ["Set", "Var1", "1", "", "", ""],
            ["Check", "Var1", "", "[OK]", "", ""],
            ["Comments", "Heading2", "", "", "REQ002", "TC002"],
            ["Set", "Var2", "2", "", "", ""],
            ["Check", "Var2", "", "[PASS]", "", ""],
        ]
        df_sample = pd.DataFrame(data, columns=columns)
        df_sample.to_excel(excel_path, index=False)
    df = pd.read_excel(excel_path)
    SIOData = []
    ReqID = ""
    Index = -1
    CommandCol = "Command"
    DataCol = "Data"
    ValueCol = "Value"
    ExpectedCol = "Expected"
    CommentCol = "Comment"
    TestcaseIDCol = "TestcaseID"
    EndRow = len(df)
    new_flag = True
    new_id = 0
    if new_flag:
        Index += 1
        SIOData.append({
            "ParHeading": excel_path.split("\\")[-1].split(".")[0],
            "TestID": new_id,
            "Heading": "New",
            "HeadingFlag": True,
            "ReqID": "",
            "Input": "",
            "Expected": "",
            "Result": "",
        })
    for i in range(1, EndRow):
        row = df.iloc[i]
        if pd.notna(row[CommentCol]):
            ReqID = row[CommentCol]
        if row[CommandCol] == "Comments":
            Index += 1
            SIOData.append({
                "Heading": row[DataCol],
                "Result": "passed",
                "Input": "",
                "Expected": "",
                "ReqID": "",
                "TestID": "",
                "HeadingFlag": "",
                "ParHeading": "",
            })
            if pd.notna(df.iloc[0][TestcaseIDCol]):
                SIOData[Index]["ReqID"] = f"{ReqID}_{df.iloc[0][TestcaseIDCol]}"
            else:
                SIOData[Index]["ReqID"] = ReqID
        else:
            if "Set" in str(row[CommandCol]):
                SIOData[Index]["Input"] += f"|Set {row[DataCol]} = {str(row[ValueCol]).replace('[','').replace(']','')}"
            if "Check" in str(row[CommandCol]):
                SIOData[Index]["Expected"] += f"|{row[DataCol]} = {str(row[ExpectedCol])}"

def bt_curdir_click():
    WorkingFolder_var.set(os.getcwd())

def bt_opendbc_click():
    path = DBC_var.get()
    if path:
        os.system(f'"C:/Program Files (x86)/Vector CANalyzer 8.1/Exec32/candb.exe" "{path}"')
    else:
        messagebox.showwarning('Thông báo', 'Chưa chọn file DBC!')

def GetDBCInfor_Button_Click():
    get_info()
    GetDBCInfo(WorkingPath, DBCPath, NodeName)



def GenerateButton_click():
    import pandas as pd
    from tkinter import messagebox
    get_info()
    # Đọc file output DBC
    excel_path = os.path.join(WorkingPath, "DBC_output.xlsx")
    if not os.path.isfile(excel_path):
        messagebox.showerror("Lỗi", f"Không tìm thấy file {excel_path}, hãy chạy Get DBC Info trước!")
        return
    try:
        df = pd.read_excel(excel_path, sheet_name="Output_dbc")
    except Exception as e:
        messagebox.showerror("Lỗi", f"Không đọc được file {excel_path}: {e}")
        return
    node_name = Node_var.get().strip()
    channel = Channel_var.get().strip().upper()
    path = WorkingFolder_var.get().strip()
    if not path.endswith("\\"):
        path += "\\"
    gen_path = os.path.join(path, "Gen.can")
    # Duyệt từng dòng, gom message TX có Node chứa NodeName, lấy chu kỳ đầu tiên, không lặp tên
    msg_dict = {}
    for idx, row in df.iterrows():
        if str(row.get("DIR", "")).strip() == "TX" and node_name in str(row.get("Node", "")):
            msg_name = str(row.get("Message", "")).strip()
            if msg_name and msg_name not in msg_dict:
                try:
                    cycle = int(float(row.get("Cyclic", 0))) if not pd.isna(row.get("Cyclic", 0)) else 0
                except:
                    cycle = 0
                msg_dict[msg_name] = cycle
    # Sắp xếp tên message
    msg_list = []
    for idx, (k, v) in enumerate(sorted(msg_dict.items(), key=lambda x: x[0])):
        msg_list.append({"Name": k, "Cycle": v, "Index": idx})
    # Sinh file Gen.can
    try:
        with open(gen_path, "w", encoding="utf-8") as f:
            f.write("/*@!Encoding:1252*/\n")
            f.write("includes\n{\n\n}\n\n")
            f.write("variables\n{\n")
            f.write("\n\t//-----------------GEN: Define MSG Start----------------------\n\n")
            for msg in msg_list:
                if channel == "PUB":
                    f.write(f"\tmessage can1.{msg['Name']}\t{msg['Name']} = {{DIR = TX}};\n")
                else:
                    f.write(f"\tmessage can2.{msg['Name']}\t{msg['Name']} = {{DIR = TX}};\n")
            f.write("\n\t//-----------------GEN: Define MSG End------------------------\n\n")
            # MSG_CONFIG_TX_ENUM
            f.write("\n\t//----------------GEN: MSG ID End-------------------\n")
            f.write("\t// Generate MSG TX CONFIG\n")
            f.write("\tenum MSG_CONFIG_TX_ENUM {\n")
            f.write("\t/* 0 */ MSG_CONFIG_TX_ENUM_START = 0,\n")
            for i, msg in enumerate(msg_list):
                f.write(f"\t/* {i+1} */\tMSG_CONFIG_TX_{msg['Name']},\n")
            f.write(f"\t/* {len(msg_list)+1} */\tMSG_CONFIG_TX_ENUM_TX_END \n")
            f.write("\t};\n")
            f.write("\n\t//----------------GEN: MSG ID End-------------------\n\n")
            # Timer
            cycles = sorted(set([m["Cycle"] for m in msg_list if m["Cycle"] > 0]))
            f.write("\n\t//----------------GEN: Define Timer Start-------------------\n\n")
            min_cycle = None
            if cycles:
                min_cycle = cycles[0]
            tMinK1 = min_cycle
            used_cycles = set()
            while tMinK1 is not None:
                f.write(f"\tmsTimer PROJ_SendMSGTimer_{tMinK1}ms;\n")
                used_cycles.add(tMinK1)
                next_cycle = 10000
                for cyc in cycles:
                    if cyc > tMinK1 and cyc < next_cycle and cyc not in used_cycles:
                        next_cycle = cyc
                if next_cycle == 10000:
                    break
                tMinK1 = next_cycle
            f.write("\n\t//----------------GEN: Define Timer End-------------------\n\n")
            f.write("}\n// varaiable End\n\n")
            # On Start
            f.write("ON Start\n{\n")
            f.write("\tPROJ_Init();\n")
            # Mapping sysvar_type đúng chuẩn
            if channel == "PUB":
                sysvar_type = "vehicle"
            elif channel == "FR":
                sysvar_type = "fr"
            elif channel == "MPC":
                sysvar_type = "mpc"
            elif channel == "CR":
                sysvar_type = "cr"
            else:
                sysvar_type = "vehicle"
            f.write(f"\tPROJ_Sim_{sysvar_type}_Signals_Init();\n")
            f.write("}\n\n")
            # PROJ_Init
            f.write("PROJ_Init ()\n{\n")
            f.write("\n\t//----------------GEN: Set Timer Start-------------------\n\t{\n")
            tMinK1 = min_cycle
            used_cycles = set()
            while tMinK1 is not None:
                f.write(f"\t\tsetTimer (PROJ_SendMSGTimer_{tMinK1}ms,  10);\n")
                used_cycles.add(tMinK1)
                next_cycle = 10000
                for cyc in cycles:
                    if cyc > tMinK1 and cyc < next_cycle and cyc not in used_cycles:
                        next_cycle = cyc
                if next_cycle == 10000:
                    break
                tMinK1 = next_cycle
            f.write("\t}\n\t//----------------GEN: Set Timer End-------------------\n\n}\n\n")
            
            # On Timer
            f.write("//----------------GEN: On Timer Start-------------------\n\n")
            # Nếu có message không có chu kỳ (cycle = 0), sinh block on key HOME
            
            msg_no_cycle = [msg for msg in msg_list if msg["Cycle"] == 0]
            if msg_no_cycle:
                f.write("on key HOME\n{\n\n\t//Calc and send MSG\n\t{\n")
                for msg in msg_no_cycle:
                    f.write(f"\t\tPROJ_CalcMsg_{msg['Name']}();\n\t\tSendMSG_{sysvar_type}(MSG_CONFIG_TX_{msg['Name']}, {msg['Name']});\n\n")
                f.write("\t}\n}\n")
            
            tMinK1 = min_cycle
            used_cycles = set()
            # Sinh các block on timer
            while tMinK1 is not None:
                f.write(f"on timer PROJ_SendMSGTimer_{tMinK1}ms\n{{\n\t//reset Timer\n\tsetTimer(PROJ_SendMSGTimer_{tMinK1}ms, {tMinK1});\n\n\t//Calc and send MSG\n\t{{\n")
                for i, msg in enumerate(msg_list):
                    if msg["Cycle"] == tMinK1:
                        f.write(f"\t\tPROJ_CalcMsg_{msg['Name']}();\n\t\tSendMSG_{sysvar_type}(MSG_CONFIG_TX_{msg['Name']}, {msg['Name']});\n\n")
                f.write("\t}\n}\n")
                used_cycles.add(tMinK1)
                next_cycle = 10000
                for cyc in cycles:
                    if cyc > tMinK1 and cyc < next_cycle and cyc not in used_cycles:
                        next_cycle = cyc
                if next_cycle == 10000:
                    break
                tMinK1 = next_cycle
            f.write("//----------------GEN: On Timer End-------------------\n")
            # Block SendMSG_
            f.write(f"\nSendMSG_{sysvar_type}(int MSGIndex, message* MSG)\n" + "{" + "\n\t\tPROJ_Sim_Timeout(MSGIndex, MSG);\n}\n")
            # Block PROJ_Sim_Timeout
            f.write(f"""PROJ_Sim_Timeout(int MSGIndex, message* MSG)
                {{
                    /* Timeout */
                    if (@Error_panel_{sysvar_type}::i_TOError[MSGIndex] != 0)
                    {{
                        if (@Error_panel_{sysvar_type}::i_TOErrorCounter[MSGIndex] > 0)
                        {{
                            @Error_panel_{sysvar_type}::i_TOErrorCounter[MSGIndex] --;
                        }}

                        if (@Error_panel_{sysvar_type}::i_TOErrorCounter[MSGIndex] == 0)
                        {{
                            @Error_panel_{sysvar_type}::i_TOError[MSGIndex] = 0;
                            @Error_panel_{sysvar_type}::i_TOErrorCounter[MSGIndex] = -1;
                        }}
                    
                    }}
                    else
                    {{
                        output(MSG);
                    }}
                }}
                """)
            # Block PROJ_Sim_<>_Signals_Init
            f.write(f"\nPROJ_Sim_{sysvar_type}_Signals_Init ()\n" + "{\n")
            for msg in msg_list:
                f.write(f"    /* {msg['Name']} */\n    {{\n")
                # Lấy tất cả signal thuộc message này và node hiện tại
                for idx, row in df.iterrows():
                    if str(row.get("DIR", "")).strip() == "TX" and str(row.get("Message", "")).strip() == msg['Name'] and node_name in str(row.get("Node", "")):
                        sig = str(row.get("Signal", "")).strip()
                        if sig:
                            f.write(f"        {msg['Name']}.{sig} = 0;\n")
                f.write("    }\n")
            f.write("}\n")

            # Block PROJ_Sim_Rollingcounter
            f.write(f"\nPROJ_Sim_Rollingcounter(int MSGIndex, message* MSG)\n" + "{\n}\n")
            # Block PROJ_Sim_Checksum
            f.write(f"\nPROJ_Sim_Checksum(int MSGIndex, message* MSG)\n" + "{\n}\n")
            f.write(f"//--------------------------------------------------\n\n")
            f.write(f"//--------------------------------------------------\n")
            # Block PROJ_CalcMsg_<message> cho từng message TX
            for msg in msg_list:
                f.write(f"\nPROJ_CalcMsg_{msg['Name']}()\n" + "{\n")
                # Lấy tất cả signal thuộc message này và node hiện tại
                for idx, row in df.iterrows():
                    if str(row.get("DIR", "")).strip() == "TX" and str(row.get("Message", "")).strip() == msg['Name'] and node_name in str(row.get("Node", "")):
                        sig = str(row.get("Signal", "")).strip()
                        if not sig:
                            continue
                        # Xác định sysvar đúng chuẩn
                        if channel == "PUB":
                            sysvar = "Vehicle_Signals"
                        elif channel == "FR":
                            sysvar = "FR_Signals"
                        elif channel == "MPC":
                            sysvar = "MPC_Signals"
                        elif channel == "CR":
                            sysvar = "CR_Signals"
                        else:
                            sysvar = "Vehicle_Signals"
                        f.write(f"    /* {sig} */\n")
                        f.write(f"    if(!@{sysvar}::i_{sig}_SIM)\n    {{\n")
                        f.write(f"        if(@{sysvar}::i_{sig}_EN)\n        {{\n")
                        f.write(f"            {msg['Name']}.{sig} = @{sysvar}::i_{sig}_Raw;\n")
                        f.write(f"        }}\n        else\n        {{\n")
                        f.write(f"            {msg['Name']}.{sig}.phys = @{sysvar}::i_{sig};\n")
                        f.write(f"        }}\n    }}\n    else\n    {{\n")
                        f.write(f"        // To be completed in simulation\n    }}\n")
                # Rolling counter, Checksum, DLC Check
                f.write(f"    /* Rolling counter */\n    PROJ_Sim_Rollingcounter(MSG_CONFIG_TX_{msg['Name']}, {msg['Name']});\n\n")
                f.write(f"    /* CheckSum */\n    PROJ_Sim_Checksum(MSG_CONFIG_TX_{msg['Name']}, {msg['Name']});\n\n")
                # Xác định sysvar_type cho lỗi DLC
                err_panel = get_err_panel_by_channel(channel)
                f.write(f"    /* DLC Check */\n    {msg['Name']}.DataLength = @Error_panel_{err_panel}::i_DLCError[MSG_CONFIG_TX_{msg['Name']}]?1:DBLookup({msg['Name']}).dlc;\n")
                f.write("}\n")
            # Gọi GenVariableSignals sau khi sinh Gen.can
            GenVariableSignals(path, sysvar_type, sysvar, msg_list)
            # Gọi Gen Main Panel
            GenMainPanel(path, sysvar_type, msg_list, main_form_name="MainPanel")
            # Gọi Gen Error Panel ngay sau Gen Main Panel
            GenErrorPanel(path, sysvar_type, msg_list)
        messagebox.showinfo("Thông báo", f"Đã Gen Thành Công")
    except Exception as e:
        messagebox.showerror("Lỗi", f"Không thể ghi file {gen_path}: {e}")
# Helper to map channel to err_panel
def get_err_panel_by_channel(channel):
    channel = str(channel).upper()
    if channel == "PUB":
        return "vehicle"
    elif channel == "FR":
        return "fr"
    elif channel == "MPC":
        return "mpc"
    elif channel == "CR":
        return "cr"
    else:
        return "vehicle"

# --- Khung hàm GetDBCInfo để bạn phát triển tiếp ---
def filter_unwanted_signals(dbc_info):
    """
    Trả về tuple (kept, removed) với kept là list các dòng giữ lại, removed là list các dòng bị loại bỏ.
    Tùy chỉnh logic filter tại đây.
    Ví dụ: loại bỏ signal có tên chứa 'RESERVED' hoặc message có tên trong blacklist.
    """
    keywords = ["CRC", "CHECKSUM", "ALIVECTR", "BLOCKCTR" ,"MSGCTR"]
    def is_remove(row):
        # 1. DIR là RX
        if str(row.get("DIR", "")).strip().upper() == "RX":
            return True
        # 2. Tên signal chứa các từ khóa (không phân biệt hoa thường)
        sig = str(row.get("Signal", "")).upper()
        for kw in keywords:
            if kw in sig:
                return True
        return False
    kept = []
    removed = []
    for row in dbc_info:
        if is_remove(row):
            removed.append(row)
        else:
            kept.append(row)
    return kept, removed

def GetDBCInfo(working_folder, dbc_path, node_name):
    """
    Đọc file DBC, phân tích nội dung và xử lý dữ liệu tương tự VBA GetDBCInfo.
    working_folder: thư mục làm việc
    dbc_path: đường dẫn file DBC
    node_name: tên node
    """
    import re
    import pandas as pd
    from tkinter import simpledialog
    try:
        from openpyxl import load_workbook, Workbook
        from openpyxl.styles import PatternFill
    except ImportError:
        load_workbook = None
        Workbook = None
        PatternFill = None

    if not os.path.isfile(dbc_path):
        messagebox.showerror("Lỗi", f"Không tìm thấy file DBC: {dbc_path}")
        return
    with open(dbc_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.read().splitlines()

    # Parse DBC
    dbc_info = []
    init_info = {}
    cycle_info = {}
    desc_info = {}
    bo_pattern = re.compile(r'^BO_ (\d+) (\w+):.* (\w+)$')
    sg_pattern = re.compile(r'^\s*SG_\s+(\w+)\s*:\s*(\d+)\|(\d+)@(\d+)([+-])\s*\(([-\d.Ee]+),([\-\d.Ee]+)\)\s*\[([\-\d.Ee]+)\|([\-\d.Ee]+)\]\s*"(.*?)"\s*(.*)$')
    ba_init_pattern = re.compile(r'^BA_ "GenSigStartValue".+SG_ \d+ (\w+) (-?[\d\.]+);')
    ba_cycle_pattern = re.compile(r'^BA_ "GenMsgCycleTime".+BO_ (\d+) (\d+);')
    val_pattern = re.compile(r'^VAL_ \d+ (\w+) (.+);')
    current_msg = None
    current_msgid = None
    current_node = None
    # Lấy biến Channel toàn cục
    global Channel
    # Hỏi người dùng có muốn xuất all node không (giống VBA)
    from tkinter import messagebox
    gen_all_node = messagebox.askyesno("Gen All Node?", "Do you want to get all Node?")
    # Lấy danh sách node từ BU_ đầu file
    bu_nodes = []
    for line in lines:
        if line.strip().startswith('BU_:'):
            bu_nodes = line.strip().split('BU_:')[1].strip().split()
            break
    for i, line in enumerate(lines):
        bo_match = bo_pattern.match(line)
        if bo_match:
            current_msgid = bo_match.group(1)
            current_msg = bo_match.group(2)
            current_node = bo_match.group(3)
            continue
        sg_match = sg_pattern.match(line)
        if sg_match and current_msg is not None:
            signal = sg_match.group(1)
            startbit = sg_match.group(2)
            length = sg_match.group(3)
            factor = sg_match.group(6)
            offset = sg_match.group(7)
            minv = sg_match.group(8)
            maxv = sg_match.group(9)
            unit = sg_match.group(10)
            node_field = sg_match.group(11)
            node_list = [n.strip() for n in node_field.split(',')] if node_field else ['']
            if gen_all_node:
                # Xuất cho tất cả node trong BU_, RX nếu current_node==node, TX ngược lại
                for node in bu_nodes:
                    if current_node == node:
                        direction = "RX"
                    else:
                        direction = "TX"
                    dbc_info.append({
                        "Node": node,
                        "Message": current_msg,
                        "MSGID": current_msgid,
                        "DIR": direction,
                        "Signal": signal,
                        "StartBit": startbit,
                        "Length": length,
                        "Factor": factor,
                        "Offset": offset,
                        "MinValue": minv,
                        "MaxValue": maxv,
                        "Unit": unit,
                        "Description": "",
                        "InitValue": 0,
                        "Cyclic": "",
                        "Norm": "",
                        "ConvFact": "",
                        "ALV/CHK_Flag": ""
                    })
            else:
                # Chỉ xuất signal cho node_name: TX nếu node_name là transmitter, RX nếu node_name là receiver
                if current_node == node_name:
                    # Nếu là transmitter, chỉ xuất RX
                    dbc_info.append({
                        "Node": node_name,
                        "Message": current_msg,
                        "MSGID": current_msgid,
                        "DIR": "RX",
                        "Signal": signal,
                        "StartBit": startbit,
                        "Length": length,
                        "Factor": factor,
                        "Offset": offset,
                        "MinValue": minv,
                        "MaxValue": maxv,
                        "Unit": unit,
                        "Description": "",
                        "InitValue": 0,
                        "Cyclic": "",
                        "Norm": "",
                        "ConvFact": "",
                        "ALV/CHK_Flag": ""
                    })
                elif node_name in node_list:
                    # Nếu là receiver (và không phải transmitter), chỉ xuất TX
                    dbc_info.append({
                        "Node": node_name,
                        "Message": current_msg,
                        "MSGID": current_msgid,
                        "DIR": "TX",
                        "Signal": signal,
                        "StartBit": startbit,
                        "Length": length,
                        "Factor": factor,
                        "Offset": offset,
                        "MinValue": minv,
                        "MaxValue": maxv,
                        "Unit": unit,
                        "Description": "",
                        "InitValue": 0,
                        "Cyclic": "",
                        "Norm": "",
                        "ConvFact": "",
                        "ALV/CHK_Flag": ""
                    })
            continue
        # Nếu không match regex, log ra console để debug
        if line.strip().startswith('SG_'):
            print('Không parse được:', line)
        ba_init_match = ba_init_pattern.match(line)
        if ba_init_match:
            init_info[ba_init_match.group(1)] = ba_init_match.group(2)
            continue
        ba_cycle_match = ba_cycle_pattern.match(line)
        if ba_cycle_match:
            cycle_info[ba_cycle_match.group(1)] = ba_cycle_match.group(2)
            continue
        val_match = val_pattern.match(line)
        if val_match:
            desc_info[val_match.group(1)] = val_match.group(2)
            continue
    # Gán init, cyclic, desc cho từng signal
    for row in dbc_info:
        sig = row["Signal"]
        msgid = row["MSGID"]
        if sig in init_info:
            try:
                row["InitValue"] = float(init_info[sig]) * float(row["Factor"]) + float(row["Offset"])
            except Exception:
                row["InitValue"] = init_info[sig]
        if msgid in cycle_info:
            row["Cyclic"] = cycle_info[msgid]
        if sig in desc_info:
            row["Description"] = desc_info[sig]
    # Sắp xếp theo Message, Signal
    dbc_info = sorted(dbc_info, key=lambda x: (x["Message"], x["Signal"]))
    # Không lọc node, xuất toàn bộ signal/message
    dbc_info = [row for row in dbc_info if row["Signal"]]

    # Lọc unwanted signals/messages, lưu lại sheet remove
    kept, removed = filter_unwanted_signals(dbc_info)

    out_path = os.path.join(working_folder, "DBC_output.xlsx")
    columns = ["Node", "Message", "DIR", "Cyclic", "Signal", "Length", "StartBit", "MinValue", "MaxValue", "Factor", "Offset", "InitValue", "Unit", "Description", "Norm", "ConvFact", "ALV/CHK_Flag"]
    import pandas as pd
    # Xóa file cũ nếu có
    if os.path.isfile(out_path):
        try:
            os.remove(out_path)
        except Exception as e:
            messagebox.showerror("Lỗi", f"Không thể xóa file {out_path}. Hãy đóng file Excel trước khi chạy lại!\n{e}")
            return
    df_keep = pd.DataFrame(kept, columns=columns)
    df_remove = pd.DataFrame(removed, columns=columns)
    # Kiểm tra file có đang bị mở không
    try:
        if load_workbook is not None:
            wb = Workbook()
            ws1 = wb.active
            ws1.title = "Output_dbc"
            for cidx, col in enumerate(columns, 1):
                ws1.cell(row=1, column=cidx, value=col)
            for ridx, row in enumerate(df_keep.values, 2):
                for cidx, val in enumerate(row, 1):
                    ws1.cell(row=ridx, column=cidx, value=val)
            # Tô màu xen kẽ cho Output_dbc
            if PatternFill:
                fill1 = PatternFill(start_color="BBD3DB", end_color="BBD3DB", fill_type="solid")
                fill2 = PatternFill(start_color="C7DF7B", end_color="C7DF7B", fill_type="solid")
                last_msg = None
                flag = True
                for ridx in range(2, len(df_keep) + 2):
                    msg = ws1.cell(row=ridx, column=2).value
                    if msg != last_msg:
                        flag = not flag
                        last_msg = msg
                    for cidx in range(1, len(columns) + 1):
                        ws1.cell(row=ridx, column=cidx).fill = fill1 if flag else fill2
            # Sheet remove
            ws2 = wb.create_sheet("remove")
            for cidx, col in enumerate(columns, 1):
                ws2.cell(row=1, column=cidx, value=col)
            for ridx, row in enumerate(df_remove.values, 2):
                for cidx, val in enumerate(row, 1):
                    ws2.cell(row=ridx, column=cidx, value=val)
            wb.save(out_path)
        else:
            # Ghi bằng pandas, sheet Output_dbc và remove
            with pd.ExcelWriter(out_path, engine="openpyxl") as writer:
                df_keep.to_excel(writer, sheet_name="Output_dbc", index=False)
                df_remove.to_excel(writer, sheet_name="remove", index=False)
    except PermissionError:
        messagebox.showerror("Lỗi", f"Không thể ghi file {out_path}. Hãy đóng file Excel trước khi chạy lại!")
        return
    messagebox.showinfo("Thông báo", f"Đã xuất dữ liệu ra file: {out_path} (sheet Output_dbc, remove)")

def commandbutton3_click():
    # SMUDoorsGen() # Placeholder
    #root.withdraw()
    def SMUDoorsGen():
        # TODO: Cập nhật code xử lý SMU Doors Gen ở đây
        pass

def commandbutton4_click():
    # SIODoorsGen() # Placeholder
    #root.withdraw()
    def SIODoorsGen():
        # TODO: Cập nhật code xử lý SIO Doors Gen ở đây
        pass

def channel_change(*args):
    global Channel
    Channel = Channel_var.get()
    save_log()

def commandbutton5_click():
    # GenDataBase() # Placeholder
    #root.withdraw()
    def GenDataBase():
        # TODO: Cập nhật code xử lý Gen DataBase ở đây
        pass

def commandbutton6_click():
    # GetSystemDegradation() # Placeholder
    #root.withdraw()
    def GetSystemDegradation():
        # TODO: Cập nhật code xử lý Get System Degradation ở đây
        pass

def commandbutton7_click():
    # GetFaultID() # Placeholder
    #root.withdraw()
    def GetFaultID():
        # TODO: Cập nhật code xử lý Get Fault ID ở đây
        pass

def dbc_change(*args):
    global DBCPath
    DBCPath = DBC_var.get()
    save_log()


def geta2l_click():
    # GetAtoL() # Placeholder
    #root.withdraw()
    def GetAtoL():
        # TODO: Cập nhật code xử lý Get A2L ở đây
        pass

def node_change(*args):
    global NodeName
    NodeName = Node_var.get()
    save_log()

def tb_RestbusPath_change(*args):
    global RestbusPath
    RestbusPath = tb_RestbusPath_var.get()
    save_log()

def workingfolder_change(*args):
    global WorkingPath
    WorkingPath = WorkingFolder_var.get()
    save_log()


# ==== GenVariableSignals: Sinh file Variable_signals_<sysvar_type>.xml (theo logic VBA) ====
def GenVariableSignals(path, sysvar_type, sysvar, msg_list):
    import os
    import pandas as pd
    var_path = os.path.join(path, f"Variable_signals_{sysvar_type}.xml")
    # Ghi header XML
    with open(var_path, "w", encoding="utf-8") as f:
        f.write('<?xml version="1.0" encoding="utf-8"?>\n')
        f.write('<systemvariables version="4">\n')
        f.write(' <namespace comment="" name="">\n')
        f.write(f'  <namespace comment="" name="{sysvar}">\n')

    # Đọc dữ liệu Output_dbc để truyền vào GenPanel
    excel_path = os.path.join(path, "DBC_output.xlsx")
    if not os.path.isfile(excel_path):
        return
    try:
        df = pd.read_excel(excel_path, sheet_name="Output_dbc")
    except Exception:
        return

    # Lấy channel từ sysvar_type
    if sysvar_type.lower() == "vehicle":
        channel = "PUB"
    else:
        channel = sysvar_type.upper()

    # Gọi GenPanel cho từng message
    for msg in msg_list:
        GenPanel(msg['Name'], path, channel, df, var_path, sysvar_type)

    # Đóng namespace
    with open(var_path, "a", encoding="utf-8") as f:
        f.write('   </namespace>\n')
        f.write('  </namespace>\n')
        f.write('</systemvariables>\n')
# Nút mở file DBC_output.xlsx
def open_dbc_output():
    path = WorkingFolder_var.get().strip()
    if not path.endswith("\\"):
        path += "\\"
    excel_path = os.path.join(path, "DBC_output.xlsx")
    if os.path.isfile(excel_path):
        try:
            os.startfile(excel_path)
        except Exception as e:
            messagebox.showerror("Lỗi", f"Không thể mở file: {e}")
    else:
        messagebox.showwarning("Thông báo", f"Không tìm thấy file: {excel_path}")

# ==== GenPanel: Sinh biến cho từng message trong XML và sinh file panel .xvp ====
def GenPanel(msg_name, path, channel, df, var_path, sysvar_type):

    # Lấy signals thuộc message này, đúng thứ tự và block như VBA
    signals = []
    SigCol = "Signal"
    NodeCol = "Node"
    LengthCol = "Length"
    MinCol = "MinValue"
    MaxCol = "MaxValue"
    InitCol = "InitValue"
    FactCol = "Factor"
    DescCol = "Description"
    ALVCHKCol = "ALV/CHK_Flag"
    UnitCol = "Unit"
    nrow = len(df)
    i = 0
    while i < nrow:
        row = df.iloc[i]
        t_MsgName = str(row.get("Message", ""))
        if t_MsgName == msg_name:
            # Bắt đầu block signals cho message này
            while i < nrow and str(df.iloc[i].get("Message", "")) == msg_name:
                row2 = df.iloc[i]
                sig = str(row2.get(SigCol, ""))
                node = str(row2.get(NodeCol, ""))
                alvchk = str(row2.get(ALVCHKCol, "")).lower()
                sig_lc = sig.lower()
                # Loại bỏ các signal không hợp lệ
                if (
                    "vector__xxx" in node.lower()
                    or "msgc" in sig_lc
                    or "alivec" in sig_lc
                    or "checksum" in sig_lc
                    or "chks" in sig_lc
                    or "blockcnt" in sig_lc
                    or "alv" in alvchk
                    or "chk" in alvchk
                ):
                    i += 1
                    continue
                if sig:
                    signals.append({
                        "Signal": sig,
                        "length": row2.get(LengthCol, ""),
                        "Min": row2.get(MinCol, ""),
                        "max": row2.get(MaxCol, ""),
                        "InitValue": row2.get(InitCol, ""),
                        "factor": row2.get(FactCol, ""),
                        "Description": row2.get(DescCol, ""),
                        "Unit": row2.get(UnitCol, "")
                    })
                i += 1
            break  # Chỉ lấy block đầu tiên của message này
        i += 1
    if not signals:
        return
    n = len(signals)
    NoHeight = int(math.sqrt(n))
    NoWidth = int(math.ceil(n / NoHeight)) if NoHeight else n
    if NoHeight * NoWidth < n:
        NoWidth += 1
    # Tạo file panel .xvp
    panel_path = os.path.join(path, f"{msg_name}.xvp")
    with open(panel_path, "w", encoding="utf-8") as f:
        f.write('<?xml version="1.0"?>\n')
        f.write('<Panel Type="Vector.CANalyzer.Panels.PanelSerializer, Vector.CANalyzer.Panels.Serializer, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null">\n')
        f.write(f' <Object Type="Vector.CANalyzer.Panels.Runtime.Panel, Vector.CANalyzer.Panels.Common, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="Panel" Children="Controls" ControlName="{msg_name}">\n')
        for i, sig in enumerate(signals):
            try:
                factor = float(sig["factor"])
            except Exception:
                factor = 1
            tType = "float" if factor < 1 else "int"
            ValueDisp = "Double" if factor < 1 else "Decimal"
            desc = str(sig["Description"])
            length = int(sig["length"]) if str(sig["length"]).isdigit() else 8
            minv = str(sig["Min"])
            maxv = str(sig["max"])
            name = sig["Signal"]
            location = f"{20 + 260 * (i % NoWidth)}, {20 + (20 + 80 + 20 + 10) * (i // NoWidth)}"
            f.write(f'  <Object Type="Vector.CANalyzer.Panels.Design.GroupBoxControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="GroupBoxControl{i}" Children="Controls" ControlName="Group Box">\n')
            if re.search(r"\-\>.+\-\>", desc) and length <= 4:
                f.write(ComboBox(str(i), location, name, minv, maxv, sysvar_type))
            else:
                f.write(Trackbar(str(i), location, name, minv, maxv, ValueDisp, sysvar_type))
            f.write(Textbox_phys(str(i), f"20, 55", name, minv, maxv, ValueDisp, sysvar_type))
            f.write(Textbox_hex(str(i), f"20, 80", name, minv, maxv, sysvar_type))
            f.write(Checkbox_R(str(i), f"180, 80", name, minv, maxv, sysvar_type))
            f.write(Checkbox_S(str(i), f"180, 55", name, minv, maxv, sysvar_type))
            f.write(f'  <Property Name="Name">GroupBoxControl{i}</Property>\n')
            f.write(f'  <Property Name="Location">{location}</Property>\n')
            f.write(f'  <Property Name="Size">240, {20 + 80 + 20 + 5}</Property>\n')
            f.write(f'  <Property Name="Text">{name}</Property>\n')
            f.write(f'  </Object>\n')
        f.write(f'  <Property Name="Text">{msg_name}</Property>\n')
        f.write(f'  <Property Name="Size">{20 + 260 * NoWidth}, {20 + (80 + 20 + 20 + 10) * NoHeight}</Property>\n')
        f.write(' </Object>\n')
        f.write('</Panel>\n')

    # Ghi biến vào file Variable_signals_<sysvar_type>.xml đúng logic VBA
    with open(var_path, "a", encoding="utf-8") as fvar:
        for sig in signals:
            name = sig["Signal"]
            init = sig["InitValue"]
            unit = sig["Unit"]
            desc = sig["Description"]
            tType = "float" if (float(sig["factor"]) if str(sig["factor"]).replace('.','',1).isdigit() else 1) < 1 else "int"
            length = sig["length"]
            fvar.write(CreateVariable(name, init, unit, desc, tType, length))
# ==== Các hàm sinh control XML cho panel (chuẩn VBA) ====
def Trackbar(ID, Location, Name, Min, max, ValueDisp, SysVar):
    tstr = f'   <Object Type="Vector.CANalyzer.Panels.Design.TrackBarControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="TrackBarControl{ID}" Children="Controls" ControlName="Track Bar">\n'
    tstr += f'    <Property Name="Name">TrackBarControl{ID}</Property>\n'
    tstr += f'    <Property Name="Size">200, 30</Property>\n'
    tstr += f'    <Property Name="Location">{Location}</Property>\n'
    if ValueDisp == "Double":
        tstr += f'    <Property Name="SmallChangeDouble">0.1</Property>\n'
    if ValueDisp == "Double":
        tstr += f'    <Property Name="MaximumDouble">{max}</Property>\n'
        tstr += f'    <Property Name="MinimumDouble">{Min}</Property>\n'
    else:
        tstr += f'    <Property Name="Maximum">{max}</Property>\n'
        tstr += f'    <Property Name="Minimum">{Min}</Property>\n'
    tstr += f'    <Property Name="SymbolConfiguration">4;16;{SysVar}_Signals;;;i_{Name};2;;;-1</Property>\n'
    tstr += f'   </Object>\n'
    return tstr

def ComboBox(ID, Location, Name, Min, max, SysVar):
    tstr = f'   <Object Type="Vector.CANalyzer.Panels.Design.ComboBoxControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="ComboBoxControl{ID}" Children="Controls" ControlName="Combo Box">\n'
    tstr += f'    <Property Name="Name">ComboBoxControl{ID}</Property>\n'
    tstr += f'    <Property Name="Size">200, 30</Property>\n'
    tstr += f'    <Property Name="Location">{Location}</Property>\n'
    tstr += f'    <Property Name="SymbolConfiguration">4;16;{SysVar}_Signals;;;i_{Name};2;;;-1</Property>\n'
    tstr += f'   </Object>\n'
    return tstr

def Textbox_phys(ID, Location, Name, Min, max, ValueDisp, SysVar):
    tstr = f'   <Object Type="Vector.CANalyzer.Panels.Design.TextBoxControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="TextBoxPhysControl{ID}" Children="Controls" ControlName="Input/Output Box">\n'
    tstr += f'    <Property Name="Name">TextBoxPhysControl{ID}</Property>\n'
    tstr += f'    <Property Name="Size">150, 20</Property>\n'
    tstr += f'    <Property Name="Location">{Location}</Property>\n'
    tstr += f'    <Property Name="ValueDisplay">{ValueDisp}</Property>\n'
    if ValueDisp == "Double":
        tstr += f'    <Property Name="ValueDecimalPlaces">1</Property>\n'
    tstr += f'    <Property Name="DescriptionText">Phys</Property>\n'
    tstr += f'    <Property Name="DescriptionSize">50, 20</Property>\n'
    tstr += f'    <Property Name="SymbolConfiguration">4;16;{SysVar}_Signals;;;i_{Name};2;;;-1</Property>\n'
    tstr += f'   </Object>\n'
    return tstr

def Textbox_hex(ID, Location, Name, Min, max, SysVar):
    tstr = f'   <Object Type="Vector.CANalyzer.Panels.Design.TextBoxControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="TextBoxControl{ID}" Children="Controls" ControlName="Input/Output Box">\n'
    tstr += f'    <Property Name="Name">TextBoxControl{ID}</Property>\n'
    tstr += f'    <Property Name="Size">150, 20</Property>\n'
    tstr += f'    <Property Name="Location">{Location}</Property>\n'
    tstr += f'    <Property Name="ValueDisplay">Hexadecimal</Property>\n'
    tstr += f'    <Property Name="DescriptionText">Hex</Property>\n'
    tstr += f'    <Property Name="DescriptionSize">50, 20</Property>\n'
    tstr += f'    <Property Name="SymbolConfiguration">4;16;{SysVar}_Signals;;;i_{Name}_Raw;1;;;-1</Property>\n'
    tstr += f'   </Object>\n'
    return tstr

def Checkbox_R(ID, Location, Name, Min, max, SysVar):
    tstr = f'   <Object Type="Vector.CANalyzer.Panels.Design.CheckBoxControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="CheckBoxControlR{ID}" Children="Controls" ControlName="Check Box">\n'
    tstr += f'    <Property Name="Name">CheckBoxControlR{ID}</Property>\n'
    tstr += f'    <Property Name="Size">55, 20</Property>\n'
    tstr += f'    <Property Name="Location">{Location}</Property>\n'
    tstr += f'    <Property Name="Text">RAW</Property>\n'
    tstr += f'    <Property Name="DescriptionText"></Property>\n'
    tstr += f'    <Property Name="SymbolConfiguration">4;16;{SysVar}_Signals;;;i_{Name}_EN;1;;;-1</Property>\n'
    tstr += f'   </Object>\n'
    return tstr

def Checkbox_S(ID, Location, Name, Min, max, SysVar):
    tstr = f'   <Object Type="Vector.CANalyzer.Panels.Design.CheckBoxControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="CheckBoxControlS{ID}" Children="Controls" ControlName="Check Box">\n'
    tstr += f'    <Property Name="Name">CheckBoxControlS{ID}</Property>\n'
    tstr += f'    <Property Name="Size">55, 20</Property>\n'
    tstr += f'    <Property Name="Location">{Location}</Property>\n'
    tstr += f'    <Property Name="Text">SIM</Property>\n'
    tstr += f'    <Property Name="DescriptionText"></Property>\n'
    tstr += f'    <Property Name="SymbolConfiguration">4;16;{SysVar}_Signals;;;i_{Name}_SIM;1;;;-1</Property>\n'
    tstr += f'   </Object>\n'
    return tstr

def CreateVariable(Name, Init, Unit, Description, tType, tLength):
    # Format Init based on tType
    try:
        if tType == "int":
            # If Init is float but integer value, cast to int
            if isinstance(Init, str) and Init.strip() != "":
                val = float(Init)
                if val.is_integer():
                    Init_fmt = str(int(val))
                else:
                    Init_fmt = str(int(round(val)))
            else:
                Init_fmt = str(int(float(Init)))
        elif tType == "float":
            val = float(Init)
            if val == 0:
                Init_fmt = "0"
            else:
                Init_fmt = f"{val:.2f}"
        else:
            Init_fmt = str(Init)
    except Exception:
        Init_fmt = str(Init)

    tstr = f'      <variable name="i_{Name}" comment="" anlyzLocal="2" type="{tType}" startValue="{Init_fmt}" readOnly="false" valueSequence="false" unit="{Unit}">\n'
    desc_str = str(Description)
    # Support both VBA style (0->MOVING|1->STATIONARY|...) and Excel style (0 "MOVING" 1 "STATIONARY" ...)
    if tLength and int(tLength) <= 4:
        # Check for Excel style: value "desc" value "desc" ...
        import re
        excel_valdesc = re.findall(r'(\d+)\s+"([^"]+)"', desc_str)
        if excel_valdesc:
            tstr += f'       <valuetable definesMinMax="false">\n'
            for val, desc in excel_valdesc:
                tstr += f'        <valuetableentry value="{val}" description="{desc}" />\n'
            tstr += f'       </valuetable>\n'
        elif "->" in desc_str:
            tstr += f'       <valuetable definesMinMax="false">\n'
            for entry in desc_str.split("|"):
                if "->" in entry:
                    parts = entry.split("->")
                    tstr += f'        <valuetableentry value="{parts[0].strip()}" description="{parts[1].strip()}" />\n'
            tstr += f'       </valuetable>\n'
    tstr += f'      </variable>\n'
    tstr += f'      <variable name="i_{Name}_Raw" comment="" anlyzLocal="2" type="int" startValue="0" readOnly="false" valueSequence="false" unit="{Unit}">\n      </variable>\n'
    tstr += f'      <variable name="i_{Name}_EN" comment="" anlyzLocal="2" type="int" startValue="0" readOnly="false" valueSequence="false" unit="{Unit}">\n      </variable>\n'
    tstr += f'      <variable name="i_{Name}_SIM" comment="" anlyzLocal="2" type="int" startValue="1" readOnly="false" valueSequence="false" unit="{Unit}">\n      </variable>\n'
    return tstr

# ==== Gen Main Panel: Sinh file MSG_Panel_<sysvar_type>.xvp (chuẩn VBA) ====
def GenMainPanel(path, sysvar_type, msg_list, main_form_name="MainPanel"):
    import os
    main_panel_path = os.path.join(path, f"MSG_Panel_{sysvar_type}.xvp")
    # Tìm maxLen tên message
    names = [msg['Name'] for msg in msg_list if msg.get('Name')]
    if not names:
        return
    maxLen = max(len(name) for name in names)
    maxWidth = 8 * maxLen
    n = len(names)
    NoHeight = int(n ** 0.5)
    NoWidth = int(n / NoHeight) if NoHeight else n
    if NoHeight * NoWidth < n:
        NoWidth += 1
    # Điều chỉnh lại theo VBA
    NoWidth = int(1000 / maxWidth) if maxWidth else 1
    if NoWidth == 0:
        NoWidth = 1
    with open(main_panel_path, "w", encoding="utf-8") as f:
        f.write('<?xml version="1.0"?>\n')
        f.write('<Panel Type="Vector.CANalyzer.Panels.PanelSerializer, Vector.CANalyzer.Panels.Serializer, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null">\n')
        f.write(f' <Object Type="Vector.CANalyzer.Panels.Runtime.Panel, Vector.CANalyzer.Panels.Common, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="Panel" Children="Controls" ControlName="{main_form_name}">\n')
        for i, name in enumerate(names):
            x = 20 + (maxWidth + 10) * (i % NoWidth)
            y = 20 + 35 * (i // NoWidth)
            f.write(PanelButtonControl(str(maxWidth), name, f"{x}, {y}") + "\n")
        f.write(f'  <Property Name="Text">{main_form_name}</Property>\n')
        # Tính lại NoHeight cho Size
        NoHeight = n // NoWidth
        if n % NoWidth:
            NoHeight += 1
        f.write(f'  <Property Name="Size">{20 + (maxWidth + 10) * NoWidth}, {20 + 35 * NoHeight}</Property>\n')
        f.write(' </Object>\n')
        f.write('</Panel>\n')

# Hàm PanelButtonControl chuẩn VBA (Python)
def PanelButtonControl(width, msg, location):
    # Remove _ from msg for Name property
    name_no_underscore = msg.replace('_', '')
    tstr = f'   <Object Type="Vector.CANalyzer.Panels.Design.PanelButtonControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="PanelButtonControl{name_no_underscore}" Children="Controls" ControlName="Panel Control Button">\n'
    tstr += f'    <Property Name="Name">PanelButtonControl{name_no_underscore}</Property>\n'
    tstr += f'    <Property Name="Size">{width}, 25</Property>\n'
    tstr += f'    <Property Name="Location">{location}</Property>\n'
    tstr += f'    <Property Name="PanelList">1;1;{msg}.xvp</Property>\n'
    tstr += f'    <Property Name="Text">{msg}</Property>\n'
    tstr += f'   </Object>'
    return tstr
# ==== Gen Error Panel: Sinh file Error_Panel_<sysvar_type>.xvp (chuẩn VBA) ====
def GenErrorPanel(path, sysvar_type, msg_list):
    import os
    error_path = os.path.join(path, f"Error_Panel_{sysvar_type}.xvp")
    error_form_name = f"Error_Panel_{sysvar_type}"
    NoWidth = 4
    n = len(msg_list)
    NoHeight = n // NoWidth
    if NoHeight * NoWidth < n:
        NoHeight += 1
    with open(error_path, "w", encoding="utf-8") as f:
        f.write('<?xml version="1.0"?>\n')
        f.write('<Panel Type="Vector.CANalyzer.Panels.PanelSerializer, Vector.CANalyzer.Panels.Serializer, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null">\n')
        f.write(f' <Object Type="Vector.CANalyzer.Panels.Runtime.Panel, Vector.CANalyzer.Panels.Common, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="Panel" Children="Controls" ControlName="{error_form_name}">\n')
        for i, msg in enumerate(msg_list):
            name = msg.get('Name')
            if not name:
                continue
            idx1 = msg.get('Index', i) + 1
            f.write(f'  <Object Type="Vector.CANalyzer.Panels.Design.GroupBoxControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="GroupBoxControl{i}" Children="Controls" ControlName="Group Box">\n')
            for j, errtype in enumerate(["TO", "CHK", "MC", "DLC"]):
                x = 10 + j * 50
                f.write(ErrCheckbox(name, f"{x}, 15", errtype, idx1, sysvar_type) + "\n")
                f.write(ErrTextbox(name, f"{x}, 35", errtype, idx1, sysvar_type) + "\n")
            f.write(f'  <Property Name="Name">GroupBoxControl{i}</Property>\n')
            f.write(f'  <Property Name="Location">{10 + 240 * (i % NoWidth)}, {20 + 60 * (i // NoWidth)}</Property>\n')
            f.write(f'  <Property Name="Size">230, 60</Property>\n')
            f.write(f'  <Property Name="Text">{name}</Property>\n')
            f.write(f'  </Object>\n')
        f.write(f'  <Property Name="Text">{error_form_name}</Property>\n')
        f.write(f'  <Property Name="Size">{20 + (10 + 240) * NoWidth}, {40 + 60 * NoHeight}</Property>\n')
        f.write(' </Object>\n')
        f.write('</Panel>\n')

# Helper: ErrCheckbox
def ErrCheckbox(msg, location, errtype, idx, sysvar_type):
    symbol_map = {
        "TO": "i_TOError",
        "CHK": "i_CHKError",
        "MC": "i_MCError",
        "DLC": "i_DLCError"
    }
    # Special case for CHK
    if errtype == "CHK":
        symbol = "i_CRC_CHKError"
    else:
        symbol = symbol_map.get(errtype, "i_TOError")
    return f'    <Object Type="Vector.CANalyzer.Panels.Design.CheckBoxControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="CheckBoxControl{errtype}{idx}" Children="Controls" ControlName="Check Box">\n' \
           f'      <Property Name="Name">CheckBoxControl{errtype}{idx}</Property>\n' \
           f'      <Property Name="Size">50, 20</Property>\n' \
           f'      <Property Name="Location">{location}</Property>\n' \
           f'      <Property Name="Text">{errtype}</Property>\n' \
           f'      <Property Name="SymbolConfiguration">4;16;Error_panel_{sysvar_type};;;{symbol};1;;;{idx}</Property>\n' \
           f'    </Object>'

# Helper: ErrTextbox
def ErrTextbox(msg, location, errtype, idx, sysvar_type):
    symbol_map = {
        "TO": "i_TOErrorCounter",
        "CHK": "i_CHKErrorCounter",
        "MC": "i_MCErrorCounter",
        "DLC": "i_DLCErrorCounter"
    }
    symbol = symbol_map.get(errtype, "i_TOErrorCounter")
    # Name: TextBoxControl<errtype>Counter<idx>
    # Size: 20, 20
    # ValueDisplay: Decimal
    # DescriptionText: (empty)
    # DescriptionSize: 20, 20
    # DisplayLabel: Hide
    # SymbolConfiguration: 4;16;Error_panel_<sysvar_type>;;;symbol;1;;;idx
    return f'    <Object Type="Vector.CANalyzer.Panels.Design.TextBoxControl, Vector.CANalyzer.Panels.CommonControls, Version=8.0.35.0, Culture=neutral, PublicKeyToken=null" Name="TextBoxControl{errtype}Counter{idx}" Children="Controls" ControlName="Input/Output Box">\n' \
           f'      <Property Name="Name">TextBoxControl{errtype}Counter{idx}</Property>\n' \
           f'      <Property Name="Size">20, 20</Property>\n' \
           f'      <Property Name="Location">{location}</Property>\n' \
           f'      <Property Name="ValueDisplay">Decimal</Property>\n' \
           f'      <Property Name="DescriptionText"></Property>\n' \
           f'      <Property Name="DescriptionSize">20, 20</Property>\n' \
           f'      <Property Name="DisplayLabel">Hide</Property>\n' \
           f'      <Property Name="SymbolConfiguration">4;16;Error_panel_{sysvar_type};;;{symbol};1;;;{idx}</Property>\n' \
           f'    </Object>'



# Giao diện Tkinter
root = tk.Tk()
root.title('RestBus AIO')

# Biến giao diện
WorkingFolder_var = tk.StringVar()
DBC_var = tk.StringVar()
Node_var = tk.StringVar()
Channel_var = tk.StringVar()
tb_RestbusPath_var = tk.StringVar()

# Load log nếu có
log_vals = load_log()
WorkingFolder_var.set(log_vals[0])
DBC_var.set(log_vals[1])
Node_var.set(log_vals[2])
Channel_var.set(log_vals[3])
tb_RestbusPath_var.set(log_vals[4])

# Liên kết sự kiện thay đổi
WorkingFolder_var.trace_add('write', workingfolder_change)
DBC_var.trace_add('write', dbc_change)
Node_var.trace_add('write', node_change)
Channel_var.trace_add('write', channel_change)
tb_RestbusPath_var.trace_add('write', tb_RestbusPath_change)

# Layout
row = 0
tk.Label(root, text='Working Folder:').grid(row=row, column=0, sticky='e')
tk.Entry(root, textvariable=WorkingFolder_var, width=40).grid(row=row, column=1)
tk.Button(root, text='Current Dir', command=bt_curdir_click).grid(row=row, column=2)
row += 1
tk.Label(root, text='DBC Path:').grid(row=row, column=0, sticky='e')
tk.Entry(root, textvariable=DBC_var, width=40).grid(row=row, column=1)
tk.Button(root, text='Open DBC', command=bt_opendbc_click).grid(row=row, column=2)
row += 1
tk.Label(root, text='Node Name:').grid(row=row, column=0, sticky='e')
tk.Entry(root, textvariable=Node_var, width=40).grid(row=row, column=1)
row += 1
tk.Label(root, text='Channel:').grid(row=row, column=0, sticky='e')
tk.Entry(root, textvariable=Channel_var, width=40).grid(row=row, column=1)
row += 1
tk.Label(root, text='Restbus Path:').grid(row=row, column=0, sticky='e')
tk.Entry(root, textvariable=tb_RestbusPath_var, width=40).grid(row=row, column=1)
row += 1

# Các nút chức năng

# Thêm nút Open DBC_output.xlsx
btns = [
    ('Get DBC Info', GetDBCInfor_Button_Click),
    ('Open Excel Output', open_dbc_output),
    ('Generate', GenerateButton_click),
    
]
for i, (text, cmd) in enumerate(btns):
    tk.Button(root, text=text, width=22, command=cmd).grid(row=row + i // 3, column=1 + i % 3, sticky='w', pady=2)

root.mainloop()
