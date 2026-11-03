from docx import Document
from docx.enum.style import WD_STYLE_TYPE
from docx.enum.table import WD_CELL_VERTICAL_ALIGNMENT, WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Inches, Pt, RGBColor


OUT_IMU = "/Users/zyj/Downloads/Kitchen_Project/STM32姿态采集终端_项目说明与面试背诵.docx"
OUT_SCOPE = "/Users/zyj/Downloads/Kitchen_Project/STM32双通道示波器_项目说明与面试背诵.docx"

FONT = "Arial Unicode MS"
MONO = "Menlo"
BLACK = RGBColor(0x00, 0x00, 0x00)
DARK = RGBColor(0x22, 0x29, 0x30)
MUTED = RGBColor(0x5D, 0x66, 0x70)
ACCENT = RGBColor(0x1F, 0x78, 0x83)
HEADER_FILL = "285B66"
ALT_FILL = "F3F7F8"
BORDER = "D9D9D9"


def set_font(run, size=10.5, bold=False, color=DARK, name=FONT):
    run.font.name = name
    rpr = run._element.get_or_add_rPr()
    rpr.rFonts.set(qn("w:ascii"), name)
    rpr.rFonts.set(qn("w:hAnsi"), name)
    rpr.rFonts.set(qn("w:eastAsia"), name)
    run.font.size = Pt(size)
    run.bold = bold
    run.font.color.rgb = color


def remove_borders(obj):
    ppr = obj._element.get_or_add_pPr()
    p_bdr = ppr.find(qn("w:pBdr"))
    if p_bdr is not None:
        ppr.remove(p_bdr)


def keep(paragraph, next_paragraph=False):
    ppr = paragraph._p.get_or_add_pPr()
    for tag in ("w:keepLines", "w:widowControl"):
        if ppr.find(qn(tag)) is None:
            ppr.append(OxmlElement(tag))
    if next_paragraph and ppr.find(qn("w:keepNext")) is None:
        ppr.append(OxmlElement("w:keepNext"))


def set_repeat_header(row):
    trpr = row._tr.get_or_add_trPr()
    if trpr.find(qn("w:tblHeader")) is None:
        trpr.append(OxmlElement("w:tblHeader"))


def prevent_row_split(row):
    trpr = row._tr.get_or_add_trPr()
    if trpr.find(qn("w:cantSplit")) is None:
        trpr.append(OxmlElement("w:cantSplit"))


def set_cell_shading(cell, fill):
    tcpr = cell._tc.get_or_add_tcPr()
    shd = tcpr.find(qn("w:shd"))
    if shd is None:
        shd = OxmlElement("w:shd")
        tcpr.append(shd)
    shd.set(qn("w:fill"), fill)


def set_cell_margins(cell, top=90, start=100, bottom=90, end=100):
    tc = cell._tc
    tcpr = tc.get_or_add_tcPr()
    mar = tcpr.first_child_found_in("w:tcMar")
    if mar is None:
        mar = OxmlElement("w:tcMar")
        tcpr.append(mar)
    for name, value in (("top", top), ("start", start), ("bottom", bottom), ("end", end)):
        node = mar.find(qn(f"w:{name}"))
        if node is None:
            node = OxmlElement(f"w:{name}")
            mar.append(node)
        node.set(qn("w:w"), str(value))
        node.set(qn("w:type"), "dxa")


def set_table_borders(table):
    tblpr = table._tbl.tblPr
    borders = tblpr.find(qn("w:tblBorders"))
    if borders is None:
        borders = OxmlElement("w:tblBorders")
        tblpr.append(borders)
    for edge in ("top", "left", "bottom", "right", "insideH", "insideV"):
        el = borders.find(qn(f"w:{edge}"))
        if el is None:
            el = OxmlElement(f"w:{edge}")
            borders.append(el)
        el.set(qn("w:val"), "single")
        el.set(qn("w:sz"), "6")
        el.set(qn("w:space"), "0")
        el.set(qn("w:color"), BORDER)


def configure_document(title_text, footer_text):
    doc = Document()
    section = doc.sections[0]
    section.page_width = Inches(8.5)
    section.page_height = Inches(11)
    section.top_margin = Inches(0.65)
    section.bottom_margin = Inches(0.62)
    section.left_margin = Inches(0.72)
    section.right_margin = Inches(0.72)
    section.header_distance = Inches(0.3)
    section.footer_distance = Inches(0.3)

    normal = doc.styles["Normal"]
    normal.font.name = FONT
    normal._element.rPr.rFonts.set(qn("w:ascii"), FONT)
    normal._element.rPr.rFonts.set(qn("w:hAnsi"), FONT)
    normal._element.rPr.rFonts.set(qn("w:eastAsia"), FONT)
    normal.font.size = Pt(10.5)
    normal.font.color.rgb = DARK
    normal.paragraph_format.line_spacing = 1.13
    normal.paragraph_format.space_after = Pt(4)

    title = doc.styles["Title"]
    title.font.name = FONT
    title._element.rPr.rFonts.set(qn("w:ascii"), FONT)
    title._element.rPr.rFonts.set(qn("w:hAnsi"), FONT)
    title._element.rPr.rFonts.set(qn("w:eastAsia"), FONT)
    title.font.size = Pt(21)
    title.font.bold = True
    title.font.color.rgb = BLACK
    title.paragraph_format.alignment = WD_ALIGN_PARAGRAPH.CENTER
    title.paragraph_format.space_after = Pt(5)
    remove_borders(title)

    for style_name, size, before, after in (("Heading 1", 14, 10, 4), ("Heading 2", 11.8, 7, 3)):
        style = doc.styles[style_name]
        style.font.name = FONT
        style._element.rPr.rFonts.set(qn("w:ascii"), FONT)
        style._element.rPr.rFonts.set(qn("w:hAnsi"), FONT)
        style._element.rPr.rFonts.set(qn("w:eastAsia"), FONT)
        style.font.size = Pt(size)
        style.font.bold = True
        style.font.color.rgb = BLACK
        style.paragraph_format.space_before = Pt(before)
        style.paragraph_format.space_after = Pt(after)
        style.paragraph_format.keep_with_next = True

    bullet = doc.styles.add_style("Guide Bullet", WD_STYLE_TYPE.PARAGRAPH)
    bullet.base_style = normal
    bullet.paragraph_format.left_indent = Inches(0.24)
    bullet.paragraph_format.first_line_indent = Inches(-0.18)
    bullet.paragraph_format.space_after = Pt(2.6)
    bullet.paragraph_format.line_spacing = 1.1

    numbered = doc.styles.add_style("Guide Number", WD_STYLE_TYPE.PARAGRAPH)
    numbered.base_style = normal
    numbered.paragraph_format.left_indent = Inches(0.28)
    numbered.paragraph_format.first_line_indent = Inches(-0.25)
    numbered.paragraph_format.space_after = Pt(3)

    code_style = doc.styles.add_style("Guide Code", WD_STYLE_TYPE.PARAGRAPH)
    code_style.base_style = normal
    code_style.font.name = MONO
    code_style._element.rPr.rFonts.set(qn("w:ascii"), MONO)
    code_style._element.rPr.rFonts.set(qn("w:hAnsi"), MONO)
    code_style._element.rPr.rFonts.set(qn("w:eastAsia"), FONT)
    code_style.font.size = Pt(9.4)
    code_style.paragraph_format.left_indent = Inches(0.28)
    code_style.paragraph_format.right_indent = Inches(0.12)
    code_style.paragraph_format.space_before = Pt(2)
    code_style.paragraph_format.space_after = Pt(5)
    code_style.paragraph_format.line_spacing = 1.05

    p = doc.add_paragraph(style="Title")
    remove_borders(p)
    set_font(p.add_run(title_text), 21, True, BLACK)
    keep(p, True)

    footer = section.footer
    fp = footer.paragraphs[0]
    fp.alignment = WD_ALIGN_PARAGRAPH.CENTER
    set_font(fp.add_run(footer_text + "  第 "), 8.5, False, MUTED)
    run = fp.add_run()
    fld_begin = OxmlElement("w:fldChar")
    fld_begin.set(qn("w:fldCharType"), "begin")
    instr = OxmlElement("w:instrText")
    instr.set(qn("xml:space"), "preserve")
    instr.text = " PAGE "
    fld_end = OxmlElement("w:fldChar")
    fld_end.set(qn("w:fldCharType"), "end")
    run._r.append(fld_begin)
    run._r.append(instr)
    run._r.append(fld_end)
    set_font(run, 8.5, False, MUTED)
    set_font(fp.add_run(" 页"), 8.5, False, MUTED)

    doc.core_properties.title = title_text
    doc.core_properties.author = "张钰捷"
    doc.core_properties.subject = "STM32项目面试准备"
    return doc


def add_intro(doc, text):
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.left_indent = Inches(0.35)
    p.paragraph_format.right_indent = Inches(0.35)
    p.paragraph_format.space_after = Pt(8)
    set_font(p.add_run(text), 10.3, False, MUTED)
    keep(p)


def add_heading(doc, text, level=1):
    p = doc.add_paragraph(style=f"Heading {level}")
    set_font(p.add_run(text), 14 if level == 1 else 11.8, True, BLACK)
    keep(p, True)
    return p


def add_label_paragraph(doc, label, text):
    p = doc.add_paragraph()
    set_font(p.add_run(label), 10.5, True, ACCENT)
    set_font(p.add_run(text), 10.5, False, DARK)
    keep(p)
    return p


def add_bullets(doc, items):
    for item in items:
        p = doc.add_paragraph(style="Guide Bullet")
        set_font(p.add_run("•  "), 10.5, True, ACCENT)
        set_font(p.add_run(item), 10.5, False, DARK)
        keep(p)


def add_numbered(doc, items):
    for i, item in enumerate(items, 1):
        p = doc.add_paragraph(style="Guide Number")
        set_font(p.add_run(f"{i}. "), 10.5, True, ACCENT)
        set_font(p.add_run(item), 10.5, False, DARK)
        keep(p)


def add_code(doc, lines):
    p = doc.add_paragraph(style="Guide Code")
    for idx, line in enumerate(lines):
        r = p.add_run(line + ("\n" if idx < len(lines) - 1 else ""))
        set_font(r, 9.4, False, DARK, MONO if line.isascii() else FONT)
    keep(p)


def add_table(doc, headers, rows, widths=None):
    table = doc.add_table(rows=1, cols=len(headers))
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.autofit = False
    set_table_borders(table)
    header = table.rows[0]
    set_repeat_header(header)
    prevent_row_split(header)
    for i, value in enumerate(headers):
        cell = header.cells[i]
        set_cell_shading(cell, HEADER_FILL)
        set_cell_margins(cell)
        cell.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER
        if widths:
            cell.width = Inches(widths[i])
        p = cell.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.paragraph_format.space_after = Pt(0)
        set_font(p.add_run(value), 9.4, True, RGBColor(0xFF, 0xFF, 0xFF))
    for r_idx, row_values in enumerate(rows):
        row = table.add_row()
        prevent_row_split(row)
        for i, value in enumerate(row_values):
            cell = row.cells[i]
            if r_idx % 2 == 1:
                set_cell_shading(cell, ALT_FILL)
            set_cell_margins(cell)
            cell.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER
            if widths:
                cell.width = Inches(widths[i])
            p = cell.paragraphs[0]
            p.paragraph_format.space_after = Pt(0)
            p.paragraph_format.line_spacing = 1.08
            set_font(p.add_run(value), 9.2, False, DARK)
    after = doc.add_paragraph()
    after.paragraph_format.space_after = Pt(0)
    return table


def add_page_break(doc):
    doc.add_page_break()


def add_shared_section(doc, other_name):
    add_heading(doc, "11 两个项目的互通点", 1)
    add_label_paragraph(
        doc,
        "统一定位  ",
        "这两个项目不是互不相关的作品，而是同一套STM32实时数据采集框架在中低速数字传感器和高速模拟采样两个场景中的落地。",
    )
    add_table(
        doc,
        ["共同能力", "姿态采集项目", "示波器项目", "面试表达"],
        [
            ["时间基准", "TIM2每10ms触发采集", "TIM3按目标采样率触发ADC", "都用硬件定时器保证时间确定性"],
            ["中断原则", "ISR只通知采集任务", "DMA ISR只切换缓冲并通知任务", "中断短小，耗时处理放到任务"],
            ["生产消费", "传感器生产，算法和存储消费", "DMA生产，处理和通信消费", "都要解决生产者和消费者速率不匹配"],
            ["缓冲机制", "消息队列加环形缓冲", "DMA双缓冲加触发环形窗口", "前者偏连续日志，后者偏高速块数据"],
            ["可靠性", "超时重试、丢帧计数、缓存水位", "CRC16、序号、Overrun计数", "异常必须可检测、可统计、可恢复"],
            ["上位机", "Python分析CSV和滤波效果", "Python解析串口并绘制波形", "共用协议解析、数据模型和绘图框架"],
        ],
        [1.15, 1.75, 1.75, 2.25],
    )
    add_heading(doc, "可以复用的代码", 2)
    add_code(
        doc,
        [
            "Common/Inc/ring_buffer.h       通用环形缓冲",
            "Common/Src/protocol.c          帧头 长度 序号 CRC16",
            "Common/Src/timebase.c          时间戳和周期统计",
            "Common/Src/diagnostics.c       丢帧 错误码 缓存水位",
            "Common/Src/log.c               分级串口日志",
            "Host/serial_parser.py          Python串口解析",
            "Host/plot_common.py            通用曲线显示",
        ],
    )
    add_heading(doc, "统一的面试讲法", 2)
    add_label_paragraph(
        doc,
        "背诵稿  ",
        f"我先通过当前项目掌握了定时采集、FreeRTOS任务解耦、缓冲区和Python数据分析。之后在{other_name}中复用了相同的分层框架，但把数据源和实时性要求进行了升级。两个项目共同解决的是采集、处理、存储或通信速度不一致的问题，区别在于一个侧重传感器融合与可靠记录，另一个侧重ADC、DMA、触发和高速数据通路。",
    )
    add_heading(doc, "需要统一掌握的基础问题", 2)
    add_bullets(
        doc,
        [
            "为什么ISR必须短小，哪些FreeRTOS API可以在ISR中调用。",
            "消息队列、任务通知、互斥量、环形缓冲和DMA双缓冲分别解决什么问题。",
            "如何测量采样周期、任务执行时间、丢帧率、缓冲区最高水位和通信吞吐量。",
            "volatile只能抑制编译器优化，不能自动保证复合操作原子性，也不能代替锁。",
            "为什么所有动态性能数据必须来自逻辑分析仪、计数器或上位机统计，而不能凭感觉填写。",
        ],
    )


def add_40_minute_plan(doc, project_kind):
    add_heading(doc, "12 约40分钟技术面的应对脚本", 1)
    add_label_paragraph(
        doc,
        "总体策略  ",
        "40分钟面试不需要连续讲40分钟。每次先用60—90秒给结论和主线，再停下来让面试官追问；所有回答尽量遵循“结论—原因—实现—验证”四步，单题控制在40—90秒。",
    )
    add_table(
        doc,
        ["时间", "可能内容", "你的目标"],
        [
            ["0—3分钟", "自我介绍、求职方向", "把话题引向STM32、FreeRTOS和实时数据采集"],
            ["3—8分钟", "教育经历、技能核验", "C语言、外设、RTOS各给一个项目证据，不罗列名词"],
            ["8—21分钟", "主项目连续深挖", "讲清数据链路、核心矛盾、设计选择、故障与验证"],
            ["21—30分钟", "第二项目及项目互通", "主动说明复用框架与实时性差异，避免像两个拼凑项目"],
            ["30—36分钟", "C/STM32/RTOS基础题", "把基础题重新落到项目：中断、DMA、队列、volatile、内存"],
            ["36—40分钟", "反问、补充", "询问岗位芯片平台、RTOS、调试方式和实习生交付范围"],
        ],
        [1.15, 2.25, 3.15],
    )

    if project_kind == "imu":
        add_heading(doc, "主项目13分钟追问链：姿态采集终端", 2)
        rows = [
            ["第1层：做了什么", "为什么做这个项目？完整数据链路是什么？", "90秒介绍，明确100Hz、MPU6050、FreeRTOS、FatFS和Python验证"],
            ["第2层：为什么这样设计", "为什么不用while轮询？为什么采集和写卡分任务？", "定时确定性；隔离SD卡长尾时延；优先级与缓冲削峰"],
            ["第3层：实现细节", "队列里放什么？环形缓冲如何判满？互补滤波怎么算？", "说出数据结构、读写指针、时间戳、0.98/0.02及适用边界"],
            ["第4层：异常与调试", "I2C卡死、SD写入阻塞、队列满怎么办？", "超时恢复、错误计数、批量写入、水位告警和可降级策略"],
            ["第5层：如何证明", "100Hz稳定吗？丢帧怎么测？滤波有效吗？", "GPIO/DWT测周期，序号测丢帧，Python对比波形；只报实测数据"],
            ["第6层：还能怎么改", "更高采样率或产品化如何做？", "DMA I2C、预分配文件、双缓冲、传感器自检、参数持久化"],
        ]
    else:
        add_heading(doc, "主项目13分钟追问链：双通道数字示波器", 2)
        rows = [
            ["第1层：做了什么", "为什么做这个项目？完整数据链路是什么？", "90秒介绍，明确TIM3 TRGO、ADC扫描、DMA双缓冲、触发与Python"],
            ["第2层：为什么这样设计", "为什么不用软件启动ADC？为什么要DMA双缓冲？", "硬件时间基准降低抖动；采集与处理并行，避免CPU逐点搬运"],
            ["第3层：实现细节", "双通道数据怎么排列？前触发怎么保留？触发状态机是什么？", "交错序列、循环缓冲、ARMED/TRIGGERED/DONE和前后触发窗口"],
            ["第4层：异常与调试", "DMA覆盖、ADC Overrun、串口半包和CRC失败怎么办？", "消费进度检查、计数与复位、状态机解析、序号和CRC16"],
            ["第5层：如何证明", "采样率准吗？能测多高频率？串口带宽够吗？", "定时器计算加GPIO/信号源验证；100kHz双通道约400KB/s，不能硬说实时全传"],
            ["第6层：还能怎么改", "更高采样率或产品化如何做？", "提高ADC时钟、USB/以太网、外部RAM、模拟前端保护和校准"],
        ]
    add_table(doc, ["追问层次", "面试官可能问", "回答必须落到"], rows, [1.55, 2.4, 2.7])

    add_heading(doc, "连续追问时的固定回答模板", 2)
    add_table(
        doc,
        ["步骤", "可直接套用的话术"],
        [
            ["先下结论", "“我这里的核心选择是……，主要为了解决……”"],
            ["解释约束", "“当时的约束是采样周期、CPU占用、内存或接口带宽，因此不能简单……”"],
            ["说到实现", "“具体用定时器/DMA/任务通知/环形缓冲实现，关键状态或数据结构是……”"],
            ["给出验证", "“我用GPIO/DWT/序号/上位机统计验证，结果应填写自己的实测值。”"],
            ["补充边界", "“该方案在当前速率下成立；如果提升到……，我会改为……”"],
        ],
        [1.3, 5.35],
    )
    add_heading(doc, "40分钟面试前的两轮练习", 2)
    add_bullets(
        doc,
        [
            "第一轮限时20分钟：自我介绍2分钟，当前项目90秒介绍，再按六层追问链回答；全程录音并删除重复表述。",
            "第二轮限时40分钟：让同学按本节顺序随机打断，任何技术选择都继续追问“为什么、怎么实现、怎么验证、失败怎么办”。",
            "遇到不会的问题先明确边界：说出已知事实、当前判断和验证方法，不编造寄存器值、指标或测试结果。",
            "准备三份证据：一张系统框图、一张关键波形或数据图、一个真实故障记录；面试官追问时能迅速展开。",
        ],
    )
    add_heading(doc, "真实性边界", 2)
    add_bullets(
        doc,
        [
            "开源仓库提供的是参考实现，面试时应明确说自己完成了哪些移植、重构、异常处理和测试。",
            "没有实际跑通的模块不要描述为已完成；没有测量的指标不要写成实测结果。",
            "至少准备实物照片、运行视频、代码仓库、波形或CSV数据，以及一个真实排障过程。",
        ],
    )


def build_imu_doc():
    doc = configure_document(
        "STM32 实时姿态采集终端 项目说明与面试背诵",
        "STM32姿态采集终端面试说明",
    )
    add_intro(
        doc,
        "用于嵌入式软件实习面试准备。先熟记快速讲稿，再理解数据链路、任务划分、滤波、存储和异常处理。文中的参数是推荐实现基线，投递前必须以自己的实测结果为准。",
    )

    add_heading(doc, "1 快速背诵版本", 1)
    add_heading(doc, "30秒版本", 2)
    add_label_paragraph(
        doc,
        "背诵稿  ",
        "我基于STM32F407和MPU6050实现了一个实时姿态采集与数据记录终端。系统使用TIM2每10毫秒唤醒采集任务，以100Hz读取六轴数据，通过零偏校准、低通和互补滤波计算横滚角与俯仰角。采集、算法、存储和显示由FreeRTOS任务解耦，并用消息队列和环形缓冲吸收SD卡写入抖动，最终以20Hz保存CSV，同时通过OLED、串口和Python工具展示与分析数据。",
    )
    add_heading(doc, "90秒版本", 2)
    add_label_paragraph(
        doc,
        "背诵稿  ",
        "这个项目的核心问题不是把MPU6050数据读出来，而是保证周期采集不被SD卡和显示操作阻塞。我使用TIM2产生10毫秒周期中断，中断中只通过FromISR接口通知高优先级采集任务，实际I2C读取放在任务上下文中。采样快照通过队列交给算法模块，算法完成零偏补偿、低通和0.98比0.02的互补滤波，再把结果放入环形缓冲。存储任务根据缓存水位批量调用FatFS写卡，显示任务低频刷新OLED。这样即使SD卡出现几十到几百毫秒的时延，采集链路仍可继续运行。系统还统计I2C超时、丢帧和缓存最高水位，并通过Python对比原始和滤波曲线。",
    )
    add_heading(doc, "3分钟版本", 2)
    for para_text in [
        "项目背景是做一个可独立运行的运动姿态记录终端，要求既能实时显示，又能把数据保存下来供离线分析。硬件选用STM32F407、MPU6050、SPI接口SD卡和I2C接口OLED，软件使用HAL、FreeRTOS、FatFS以及Python。",
        "启动阶段先初始化时钟、GPIO、串口、I2C、SPI和文件系统，再读取WHO_AM_I确认传感器在线。设备静止时采集1000组陀螺仪数据求平均，作为零偏。运行阶段由TIM2提供100Hz时间基准，采集任务一次读取连续14字节的加速度、温度和角速度寄存器，然后完成单位换算和滤波。",
        "架构上将采集、算法、存储和显示拆开。队列负责传递完整采样快照，环形缓冲负责吸收存储任务的瞬时阻塞。姿态计算使用互补滤波，用陀螺仪积分响应快速变化，用加速度角度修正长期漂移。存储端每5个采样点记录一次，以20Hz写入CSV，并采用批量写入和周期性f_sync平衡性能与掉电安全。",
        "我重点验证了采样周期、任务执行时间、缓冲区水位和丢帧数。遇到I2C超时会记录错误并尝试重新初始化总线；SD卡异常时停止写卡但保留串口输出。项目最关键的设计价值是把实时采集和非确定时延的存储操作解耦，而不是简单调用传感器库。",
    ]:
        p = doc.add_paragraph()
        set_font(p.add_run(para_text), 10.5, False, DARK)
        keep(p)

    add_heading(doc, "2 项目目标与技术栈", 1)
    add_table(
        doc,
        ["类别", "内容", "作用"],
        [
            ["主控", "STM32F407 Cortex M4", "运行FreeRTOS、姿态算法、文件系统和通信"],
            ["传感器", "MPU6050 六轴IMU", "提供三轴加速度、三轴角速度和温度"],
            ["外设", "microSD、SSD1306或SH1106 OLED", "CSV记录和本地状态显示"],
            ["底层", "GPIO、TIM2、I2C、SPI、UART、NVIC", "构建采集、存储和调试数据通路"],
            ["系统", "FreeRTOS、消息队列、任务通知、互斥量", "组织不同实时性任务并完成同步"],
            ["中间件", "FatFS、环形缓冲、日志模块", "文件写入、削峰和异常记录"],
            ["算法", "零偏校准、低通、互补滤波", "降低噪声并估算横滚角与俯仰角"],
            ["上位机", "Python、Pandas、Matplotlib", "检查采样周期并对比滤波前后曲线"],
        ],
        [1.05, 2.25, 3.45],
    )
    add_heading(doc, "项目指标基线", 2)
    add_bullets(
        doc,
        [
            "采样频率100Hz，对应名义采样周期10ms。",
            "CSV记录频率20Hz，即每5个采样点记录1次。",
            "OLED建议2Hz至10Hz刷新，避免无意义占用总线和CPU。",
            "连续运行期间统计采样次数、丢帧数、I2C错误数和环形缓冲最高水位。",
            "项目完成后用实测数据补充周期抖动、最长写卡时延和滤波前后噪声。",
        ],
    )

    add_page_break(doc)
    add_heading(doc, "3 整体框架与数据流程", 1)
    add_label_paragraph(doc, "数据链路  ", "TIM2中断 -> 任务通知 -> 采集任务 -> MPU6050 -> 算法任务 -> 环形缓冲 -> FatFS -> CSV -> Python")
    add_table(
        doc,
        ["层次", "主要模块", "职责"],
        [
            ["硬件层", "STM32F407、MPU6050、SD卡、OLED", "产生、保存并显示数据"],
            ["BSP层", "时钟、GPIO、TIM2、NVIC", "完成与开发板有关的初始化"],
            ["驱动层", "mpu6050、sd_spi、oled、uart", "封装寄存器读写和设备操作"],
            ["服务层", "filter、ring_buffer、fatfs_logger、diagnostics", "算法、缓冲、记录和监控"],
            ["任务层", "acquisition、fusion、storage、display、monitor", "按照实时性组织业务流程"],
            ["上位机层", "CSV分析和曲线绘制", "验证采样、滤波和系统稳定性"],
        ],
        [1.1, 2.7, 2.95],
    )
    add_heading(doc, "推荐目录结构", 2)
    add_code(
        doc,
        [
            "Core/Inc and Core/Src          CubeMX生成及系统入口",
            "Bsp/timebase.c                 TIM2时间基准",
            "Drivers/mpu6050.c              MPU6050寄存器驱动",
            "Drivers/sd_spi.c               SD卡SPI接口",
            "Services/fusion.c              低通和互补滤波",
            "Services/ring_buffer.c         无动态内存环形缓冲",
            "Services/fatfs_logger.c        CSV批量记录",
            "Services/diagnostics.c         丢帧和错误统计",
            "App/app_tasks.c                FreeRTOS任务与优先级",
            "Host/analyze.py                Python离线分析",
        ],
    )

    add_heading(doc, "4 完整运行流程", 1)
    add_heading(doc, "启动阶段", 2)
    add_numbered(
        doc,
        [
            "初始化系统时钟，配置SysTick和FreeRTOS时基。",
            "初始化UART，保证后续错误可以输出日志。",
            "初始化I2C并读取MPU6050的WHO_AM_I寄存器，确认地址和器件身份。",
            "唤醒MPU6050，设置量程、数字低通滤波和采样分频。",
            "初始化SPI和FatFS，挂载SD卡并创建带表头的CSV文件。",
            "初始化OLED并显示传感器、SD卡和系统状态。",
            "要求设备静止，采集1000组陀螺仪数据计算零偏。",
            "创建队列、环形缓冲和各任务，最后启动TIM2采样时基。",
        ],
    )
    add_heading(doc, "运行阶段", 2)
    add_numbered(
        doc,
        [
            "TIM2每10ms产生一次更新中断。",
            "中断使用vTaskNotifyGiveFromISR通知采集任务，并按需触发一次任务切换。",
            "采集任务连续读取MPU6050的14字节数据，附加时间戳和序号。",
            "读取成功后将采样快照发送到算法队列；失败则增加错误计数并保留上次有效状态。",
            "算法任务完成单位换算、零偏补偿、低通和互补滤波。",
            "结果写入日志环形缓冲，并更新供OLED读取的最新姿态快照。",
            "存储任务按水位或超时条件批量写CSV，每隔一定时间调用f_sync。",
            "显示任务低频更新姿态角、采样频率、SD卡状态和错误计数。",
            "监控任务每秒计算实际采样率、丢帧率和缓存最高水位。",
        ],
    )

    add_heading(doc, "5 任务与数据结构设计", 1)
    add_table(
        doc,
        ["执行单元", "周期或触发", "优先级", "主要工作", "设计理由"],
        [
            ["TIM2 ISR", "10ms", "中断", "发送任务通知", "保证ISR短小"],
            ["采集任务", "任务通知", "高", "I2C读取和时间戳", "优先保证采样周期"],
            ["算法任务", "采样队列", "中高", "校准和姿态滤波", "与I2C操作解耦"],
            ["存储任务", "水位或超时", "中低", "FatFS批量写入", "允许被实时任务抢占"],
            ["显示任务", "200ms至500ms", "低", "OLED刷新", "显示不应影响采样"],
            ["监控任务", "1s", "低", "频率和错误统计", "提供可观测性"],
        ],
        [1.05, 1.1, 0.7, 2.05, 2.0],
    )
    add_heading(doc, "采样结构体建议", 2)
    add_code(
        doc,
        [
            "typedef struct {",
            "    uint32_t tick_ms;",
            "    uint32_t sequence;",
            "    int16_t ax_raw, ay_raw, az_raw;",
            "    int16_t gx_raw, gy_raw, gz_raw;",
            "    float roll_deg, pitch_deg;",
            "    uint16_t status_flags;",
            "} imu_sample_t;",
        ],
    )

    add_page_break(doc)
    add_heading(doc, "6 关键技术与设计原因", 1)
    add_heading(doc, "MPU6050配置与读取", 2)
    add_bullets(
        doc,
        [
            "AD0接低时7位I2C地址通常为0x68，接高时为0x69；HAL接口常要求左移一位后传入。",
            "WHO_AM_I寄存器地址0x75，常见返回值0x68；PWR_MGMT_1地址0x6B，用于唤醒和时钟源设置。",
            "加速度数据从0x3B开始，连续读取14字节可获得加速度、温度和角速度，连续读取比逐寄存器读取开销小。",
            "若使用正负2g量程，加速度换算系数通常为16384 LSB/g；正负250度每秒量程的陀螺仪系数通常为131 LSB/(度每秒)。",
        ],
    )
    add_heading(doc, "零偏和互补滤波", 2)
    add_code(
        doc,
        [
            "gyro_bias = sum(gyro_raw[0..999]) / 1000",
            "gyro_corrected = gyro_raw - gyro_bias",
            "angle_gyro = angle_prev + gyro_rate * 0.01",
            "angle = 0.98 * angle_gyro + 0.02 * angle_acc",
        ],
    )
    add_bullets(
        doc,
        [
            "陀螺仪短期响应快，但积分后会漂移；加速度计长期稳定，但运动和振动时噪声大。",
            "互补滤波使用陀螺仪承担高频变化，使用加速度角度修正低频漂移。",
            "0.98和0.02是起始参数，应结合采样周期、传感器噪声和动态响应实测调整。",
            "MPU6050无法仅靠加速度和陀螺仪获得长期稳定的绝对航向角，航向通常还需要磁力计或外部参考。",
        ],
    )
    add_heading(doc, "环形缓冲和FatFS", 2)
    add_bullets(
        doc,
        [
            "环形缓冲使用固定数组、读指针、写指针和计数器，避免运行期动态内存碎片。",
            "容量可按采样率 x 最坏写卡阻塞时间 x 安全系数估算。例如100Hz采样、最坏阻塞200ms，理论最低20条，实际可选择64或128条。",
            "队列负责跨任务传递离散采样消息，环形缓冲负责吸收持续数据流的瞬时积压，两者用途不同。",
            "批量f_write可以减少文件系统调用；周期性f_sync提高掉电数据完整性，但过于频繁会降低性能。",
        ],
    )
    add_heading(doc, "关键设计决策表", 2)
    add_table(
        doc,
        ["设计决策", "没有这样设计的风险", "采用原因"],
        [
            ["硬件定时器触发", "延时循环受任务执行时间影响", "获得稳定时间基准"],
            ["ISR只发通知", "I2C和写卡会长时间占用中断", "控制中断延迟"],
            ["任务分层", "采集与慢速外设互相阻塞", "隔离实时性要求"],
            ["环形缓冲", "SD卡抖动直接造成漏采", "削峰并统计积压"],
            ["低频OLED刷新", "I2C总线和CPU浪费", "显示无需100Hz刷新"],
            ["错误计数", "系统异常无法复现和量化", "提高可观测性"],
        ],
        [1.6, 2.6, 2.15],
    )

    add_heading(doc, "7 异常处理与排障", 1)
    add_table(
        doc,
        ["现象", "可能原因", "定位方法", "处理策略"],
        [
            ["姿态缓慢漂移", "陀螺仪零偏或温漂", "静止记录角速度均值", "重新校准并用加速度修正"],
            ["姿态跳变", "振动、I2C错误或字节序错误", "查看原始值和错误码", "低通、重读并检查高低字节"],
            ["周期偶发变长", "SD卡写入阻塞", "GPIO翻转和缓存水位", "环形缓冲加批量写入"],
            ["I2C一直BUSY", "从机拉住SDA或总线异常", "读取状态并观察波形", "超时、重初始化、SCL恢复"],
            ["CSV末尾损坏", "掉电前未同步文件", "重复断电测试", "周期f_sync和安全停止"],
            ["队列或缓冲溢出", "消费者长期慢于生产者", "统计高水位和丢帧", "优化处理或降低记录频率"],
        ],
        [1.25, 1.75, 1.7, 2.25],
    )

    add_heading(doc, "8 测试与结果准备", 1)
    add_table(
        doc,
        ["测试项", "方法", "面试时要能给出的结果"],
        [
            ["采样频率", "采集前后翻转GPIO并用逻辑分析仪测量", "平均周期、最大抖动、60秒采样数"],
            ["任务耗时", "DWT周期计数器或GPIO脉宽", "I2C读取、算法、写卡耗时"],
            ["写卡抖动", "记录每次f_write耗时", "平均值、最大值、P95或P99"],
            ["丢帧", "序号连续性和lost_count", "长时间运行丢帧率"],
            ["滤波效果", "静止和缓慢旋转测试", "原始与滤波后的标准差和漂移"],
            ["异常恢复", "拔卡、断开传感器、模拟NACK", "系统是否死机及恢复时间"],
        ],
        [1.3, 2.6, 2.75],
    )
    add_label_paragraph(doc, "结果记录模板  ", "100Hz目标；实际平均周期____ms；最大抖动____us；最长写卡____ms；缓冲最高水位____%；运行____小时丢帧____条。")

    add_heading(doc, "9 高频面试问题与参考回答", 1)
    imu_qa = [
        ("为什么选择STM32F407", "主频和SRAM余量较充足，具备FPU、DMA和丰富定时器，能同时运行FreeRTOS、FatFS、姿态计算和显示。选择它主要是为任务解耦和后续扩展留余量。"),
        ("为什么选择100Hz", "人体动作和一般姿态变化带宽远低于100Hz，100Hz能够获得较平滑的动态响应，CPU和I2C负载也可控。最终频率应根据应用带宽和传感器ODR确定。"),
        ("为什么TIM2比vTaskDelay更合适", "vTaskDelay容易产生累计漂移，vTaskDelayUntil更好但仍依赖调度；硬件定时器提供更稳定的事件时刻，ISR再通知任务完成处理。"),
        ("中断里能不能调用普通队列API", "应调用带FromISR后缀的API，并使用portYIELD_FROM_ISR按需切换任务；不能在ISR中调用可能阻塞的普通任务API。"),
        ("为什么不在ISR里读I2C", "I2C读取可能等待、超时或重试，会延长中断时间并增加系统中断延迟，因此ISR只发送通知。"),
        ("队列为什么传结构体而不是指针", "小结构体复制能避免指针生命周期和所有权问题；若数据块很大，可使用内存池加指针，但必须明确何时归还。"),
        ("环形缓冲如何判断满和空", "可以保留一个空槽用读写指针判断，也可以额外维护count。多任务访问时需要临界区、互斥量或单生产者单消费者设计。"),
        ("缓冲区满了怎么办", "必须定义策略。本项目建议拒绝新日志并增加lost_count，保留已排队数据，同时上报溢出，不能无提示覆盖。"),
        ("为什么使用互补滤波", "它用陀螺仪提供短期动态，用加速度计修正长期漂移，计算量小、参数少，适合F407上的实时姿态显示。"),
        ("0.98参数怎么来的", "它体现对陀螺仪高频分量的权重，是初始经验值；应结合采样周期和噪声实测调节，也可从目标截止频率推导时间常数。"),
        ("为什么不能得到稳定航向角", "没有磁力计或外部参考时，绕重力方向的偏航无法由加速度计修正，陀螺仪积分会长期漂移。"),
        ("SD卡为什么会影响实时性", "Flash内部擦写、控制器缓存和FatFS元数据更新会产生不可预测长延迟，所以必须与实时采集解耦。"),
        ("为什么不是每条数据都f_sync", "每条同步最安全但开销大。采用批量写和周期同步，在性能、数据完整性和掉电风险之间平衡。"),
        ("互斥量和信号量有什么区别", "互斥量用于资源所有权并通常支持优先级继承；信号量更适合事件或资源计数。共享总线或文件句柄使用互斥量更合适。"),
        ("如何处理优先级反转", "缩短持锁区间，避免高优先级任务等待低优先级任务执行耗时工作，并使用带优先级继承的FreeRTOS互斥量。"),
        ("如何测量丢帧", "每个采样附加单调递增序号，消费者检查是否连续；同时统计通知次数、成功采样数和缓冲溢出次数。"),
        ("I2C超时后怎么恢复", "先记录错误并停止本次传输，检查BUSY和SDA状态，重新初始化I2C；必要时用GPIO输出SCL脉冲释放从机，再重新识别WHO_AM_I。"),
        ("项目中最难的问题是什么", "实时采集和SD卡不确定写入时延之间的冲突。通过任务拆分、环形缓冲、批量写入和水位监控解决，而不是简单提高任务优先级。"),
        ("你对开源项目做了什么", "应按实际情况回答，例如更换到F407、重新划分FreeRTOS任务、增加环形缓冲、错误恢复、统计指标和Python验证，不把原作者代码描述成独立原创。"),
    ]
    add_table(doc, ["问题", "参考回答"], imu_qa, [2.05, 4.65])

    add_heading(doc, "10 一分钟速记清单", 1)
    add_bullets(
        doc,
        [
            "主线：TIM2定时 -> 通知采集 -> I2C读取 -> 校准滤波 -> 环形缓冲 -> FatFS -> Python。",
            "核心矛盾：100Hz实时采集与SD卡非确定时延。",
            "核心方案：ISR短小、任务解耦、队列传消息、环形缓冲削峰、批量写入。",
            "算法：1000组零偏，低通，0.98/0.02互补滤波，只稳定估算Roll和Pitch。",
            "验证：GPIO或DWT测周期，序号测丢帧，水位测积压，Python测滤波效果。",
            "故障故事：直接写卡导致采样抖动，改为缓存和独立存储任务。",
        ],
    )
    add_shared_section(doc, "双通道数字示波器项目")
    add_40_minute_plan(doc, "imu")
    add_heading(doc, "13 参考项目", 1)
    add_bullets(doc, ["https://github.com/rohaanbrar-stack/stm32-imu-logger", "参考仓库用于理解思路；最终面试内容应以自己实际完成和测试的版本为准。"])
    doc.save(OUT_IMU)


def build_scope_doc():
    doc = configure_document(
        "STM32 双通道数字示波器 项目说明与面试背诵",
        "STM32双通道数字示波器面试说明",
    )
    add_intro(
        doc,
        "用于嵌入式软件实习面试准备。重点掌握TIM触发ADC、DMA双缓冲、触发状态机、SRAM采样窗口、串口协议和带宽计算。文中参数是推荐实现基线，必须在自己的硬件上完成验证。",
    )

    add_heading(doc, "1 快速背诵版本", 1)
    add_heading(doc, "30秒版本", 2)
    add_label_paragraph(
        doc,
        "背诵稿  ",
        "我基于STM32F407实现了一个双通道数字示波器。系统使用TIM3的更新事件触发ADC双通道扫描，通过DMA循环双缓冲连续搬运数据，在半传输和完成中断中通知处理任务。项目支持10Hz到100kHz采样率、上升沿和下降沿触发，并将触发前后波形保存在SRAM。采集完成后通过带序号和CRC16的串口协议上传，Python上位机完成波形显示以及均值、峰峰值和频率计算。",
    )
    add_heading(doc, "90秒版本", 2)
    add_label_paragraph(
        doc,
        "背诵稿  ",
        "示波器项目的核心是构建确定性的高速数据链路。TIM3通过TRGO触发ADC规则组，ADC按通道一和通道二交替转换，DMA将结果写入交错缓冲区。DMA半传输和完成回调中只标记可处理的半区并发送任务通知，处理任务再完成通道拆分、触发检测和数据统计。为了支持触发前波形，我使用循环缓冲保存最近数据；检测到阈值越过后继续采集设定数量的后触发样本，形成完整窗口。100kHz双通道16位数据需要约400KB每秒，而115200串口只有约11.5KB每秒有效吞吐，因此采用先采集到SRAM、再分帧上传，高采样率预览则进行抽点。",
    )
    add_heading(doc, "3分钟版本", 2)
    for para_text in [
        "项目目标是使用低成本STM32开发板实现可配置采样、稳定触发和PC端显示。硬件包括STM32F407、两路受保护的模拟输入、ST-Link串口以及信号源，软件使用HAL、FreeRTOS、ADC、TIM、DMA、UART和Python。输入端必须限制在ADC允许范围，真实电路应增加分压、限流和钳位保护。",
        "初始化时配置ADC规则组双通道扫描、DMA循环模式和TIM3 TRGO。采样频率由定时器时钟、预分频器PSC和自动重装值ARR决定。DMA数据按通道一、通道二交错排列；半传输和完成中断轮流通知处理任务，使CPU处理一个半区时DMA填充另一个半区。",
        "触发模块使用模拟看门狗快速产生阈值事件，再通过前一个和当前样本判断上升沿或下降沿。状态机包括IDLE、ARMED、TRIGGERED、POST_CAPTURE、DONE和UPLOAD。触发前数据来自循环缓冲，触发后继续采集固定长度，最后冻结采样窗口。",
        "通信方面使用帧头、版本、命令、序号、长度、负载和CRC16。由于UART无法实时传完最高采样率数据，高速模式采用定长采集后上传，预览模式采用抽点。Python端使用接收缓冲处理半包和粘包，解析后绘图并计算均值、峰峰值和频率。项目重点体现了定时器、ADC、DMA、中断、任务和通信带宽之间的系统设计。",
    ]:
        p = doc.add_paragraph()
        set_font(p.add_run(para_text), 10.5, False, DARK)
        keep(p)

    add_heading(doc, "2 项目目标与技术栈", 1)
    add_table(
        doc,
        ["类别", "内容", "作用"],
        [
            ["主控", "STM32F407 Cortex M4", "提供ADC、DMA、定时器、SRAM和FreeRTOS运行环境"],
            ["输入", "双路0至3.3V模拟信号", "作为待采集波形，外部需保护和调理"],
            ["采样", "TIM3、ADC规则组、DMA循环模式", "硬件触发并连续搬运双通道数据"],
            ["触发", "模拟看门狗加软件状态机", "实现阈值、上升沿、下降沿和前后触发"],
            ["系统", "FreeRTOS任务通知和任务调度", "将采集、处理、通信和控制解耦"],
            ["协议", "UART、帧头、序号、长度、CRC16", "可靠传输配置、状态和采样数据"],
            ["上位机", "Python、PySerial、Matplotlib", "配置设备、解析数据、显示和测量波形"],
        ],
        [1.05, 2.45, 3.25],
    )
    add_heading(doc, "项目指标基线", 2)
    add_bullets(
        doc,
        [
            "双通道采样，目标采样率可配置为10Hz至100kHz。",
            "支持自动、上升沿和下降沿触发，触发阈值可配置。",
            "支持前触发和后触发采样窗口，窗口长度受DMA可访问SRAM容量限制。",
            "支持均值、峰峰值、频率计算，高采样率采用采集后上传或抽点预览。",
            "持续统计DMA Overrun、串口CRC错误、丢包和任务处理超时。",
        ],
    )

    add_page_break(doc)
    add_heading(doc, "3 整体框架与数据流程", 1)
    add_label_paragraph(doc, "数据链路  ", "Python配置 -> UART命令 -> TIM3 TRGO -> ADC双通道 -> DMA双缓冲 -> 触发处理 -> SRAM窗口 -> 数据帧 -> Python显示")
    add_table(
        doc,
        ["层次", "主要模块", "职责"],
        [
            ["硬件层", "STM32F407、输入保护、信号源", "提供受控模拟输入和采样资源"],
            ["BSP层", "时钟、GPIO、TIM3、NVIC", "配置开发板和采样时间基准"],
            ["驱动层", "adc_dma、uart、analog_watchdog", "配置外设并提供中断回调"],
            ["服务层", "trigger、capture、protocol、measure", "触发窗口、协议和指标计算"],
            ["任务层", "control、process、communication、monitor", "处理配置、数据块、上传和监控"],
            ["上位机层", "serial_parser、plot、control_panel", "配置、解析、绘图和测量"],
        ],
        [1.1, 2.7, 2.95],
    )
    add_heading(doc, "推荐目录结构", 2)
    add_code(
        doc,
        [
            "Core/Inc and Core/Src          CubeMX生成及系统入口",
            "Bsp/timebase.c                 TIM3采样时基",
            "Drivers/adc_dma.c              ADC扫描和DMA双缓冲",
            "Drivers/uart.c                 串口收发和DMA发送",
            "Services/trigger.c             触发状态机",
            "Services/capture.c             前后触发采样窗口",
            "Services/protocol.c            帧头 序号 长度 CRC16",
            "Services/measurement.c         均值 峰峰值 频率",
            "App/app_tasks.c                FreeRTOS任务",
            "Host/scope_gui.py              Python上位机",
        ],
    )

    add_heading(doc, "4 完整运行流程", 1)
    add_heading(doc, "启动阶段", 2)
    add_numbered(
        doc,
        [
            "初始化系统时钟、GPIO、UART和FreeRTOS对象。",
            "配置ADC规则组为双通道扫描，并选择分辨率和采样时间。",
            "配置DMA循环模式、数据宽度、内存递增以及半传输和完成中断。",
            "配置TIM3预分频器和ARR，并将更新事件输出为TRGO。",
            "配置模拟看门狗阈值和被监控通道。",
            "创建控制、处理、通信和监控任务，但暂不启动采样。",
            "等待Python或串口CLI下发采样率、触发模式、阈值和窗口长度。",
        ],
    )
    add_heading(doc, "一次采集过程", 2)
    add_numbered(
        doc,
        [
            "验证配置是否合法，并根据目标采样率计算PSC和ARR。",
            "清空DMA状态、触发状态、序号和Overrun计数。",
            "启动DMA、ADC和TIM3，将状态切换为ARMED。",
            "DMA半传输或完成回调通知处理任务当前可读半区。",
            "处理任务拆分两个通道，并持续更新前触发环形缓冲。",
            "模拟看门狗产生阈值事件，软件比较相邻样本确认上升沿或下降沿。",
            "状态进入TRIGGERED和POST_CAPTURE，继续收集指定后触发样本。",
            "窗口完整后停止采样，状态进入DONE。",
            "通信任务将窗口分帧上传，Python验证序号和CRC后绘制波形。",
        ],
    )

    add_heading(doc, "5 任务与状态机设计", 1)
    add_table(
        doc,
        ["执行单元", "触发条件", "优先级", "主要工作", "设计理由"],
        [
            ["DMA ISR", "半传输或完成", "中断", "记录半区并发通知", "ISR保持短小"],
            ["处理任务", "任务通知", "高", "拆通道、触发和窗口管理", "必须在下次覆盖前处理"],
            ["控制任务", "UART命令", "中", "解析配置和控制状态", "不参与高速数据路径"],
            ["通信任务", "采集完成", "中低", "分帧发送采样数据", "避免阻塞采集"],
            ["监控任务", "1s", "低", "统计Overrun和吞吐量", "提供可观测性"],
        ],
        [1.05, 1.25, 0.7, 2.1, 1.8],
    )
    add_heading(doc, "触发状态机", 2)
    add_code(
        doc,
        [
            "IDLE -> ARMED -> TRIGGERED -> POST_CAPTURE -> DONE -> UPLOAD",
            "  ^                                              |",
            "  +---------------------- RESET -----------------+",
            "任何状态检测到非法配置或外设错误时进入ERROR。",
        ],
    )

    add_page_break(doc)
    add_heading(doc, "6 关键技术与设计原因", 1)
    add_heading(doc, "TIM触发ADC", 2)
    add_code(doc, ["sample_rate = TIM_CLK / ((PSC + 1) * (ARR + 1))"])
    add_bullets(
        doc,
        [
            "TIM_CLK要结合APB分频和定时器倍频规则计算，不能简单等于系统主频。",
            "ADC两个通道完成一次扫描的总转换时间必须短于一个触发周期。",
            "提高分辨率和采样时间有利于精度，但会降低可达到的最大采样率。",
            "信号最高频率必须低于采样率一半，实际还应留出余量并加入模拟抗混叠滤波。",
        ],
    )
    add_heading(doc, "DMA循环双缓冲", 2)
    add_code(
        doc,
        [
            "DMA buffer: CH1_0 CH2_0 CH1_1 CH2_1 ...",
            "Half callback: process buffer[0 .. N/2-1]",
            "Full callback: process buffer[N/2 .. N-1]",
        ],
    )
    add_bullets(
        doc,
        [
            "CPU处理前半区时DMA填充后半区，处理后半区时DMA重新填充前半区。",
            "处理必须在DMA再次覆盖该半区之前完成，否则增加Overrun计数并丢弃当前块。",
            "F407的CCM RAM通常不能作为DMA目标区，DMA缓冲应放在DMA可访问的SRAM中，并以具体芯片手册为准。",
            "F407没有F7或H7上的数据缓存一致性问题，但仍需处理ISR和任务之间的共享状态。",
        ],
    )
    add_heading(doc, "触发窗口", 2)
    add_bullets(
        doc,
        [
            "模拟看门狗快速检测采样值进入或离开阈值范围，具体边沿由previous和current样本判断。",
            "ARMED状态下持续覆盖前触发环形缓冲，触发发生时记录写指针位置。",
            "触发后继续采集固定数量样本，再将环形数据按时间顺序整理为连续窗口。",
            "窗口大小由可用SRAM决定。例如4096个时间点、双通道、每点每通道2字节，需要约16KB。",
        ],
    )
    add_heading(doc, "UART带宽与协议", 2)
    add_code(
        doc,
        [
            "raw_rate = 100000 samples/s * 2 channels * 2 bytes = 400000 B/s",
            "UART_8N1 = 115200 bit/s / 10 bit/byte = 11520 B/s",
            "结论: 115200串口无法实时传输100kHz双通道原始数据。",
            "frame = magic + version + command + sequence + length + payload + CRC16",
        ],
    )
    add_bullets(
        doc,
        [
            "高速模式采用先采集到SRAM，再停止采样并分帧上传。",
            "实时预览采用抽点、降低采样率或只上传统计值，后续可升级USB CDC。",
            "帧头用于重新同步，长度用于判断完整报文，序号用于发现丢包，CRC16用于检测随机传输错误。",
            "CRC不提供认证能力，不能防止恶意修改；若扩展到安全升级，应使用消息认证或数字签名。",
        ],
    )
    add_heading(doc, "关键设计决策表", 2)
    add_table(
        doc,
        ["设计决策", "没有这样设计的风险", "采用原因"],
        [
            ["TIM TRGO触发ADC", "软件启动造成采样抖动", "采样时刻由硬件确定"],
            ["DMA双缓冲", "逐点中断占用大量CPU", "降低中断频率并块处理"],
            ["模拟看门狗", "任务轮询增加触发延迟", "硬件快速发现阈值事件"],
            ["前触发环形窗口", "只能看到触发后的波形", "保留事件发生前历史"],
            ["采集后上传", "UART吞吐量远低于原始数据率", "避免通信阻塞采样"],
            ["序号和CRC16", "无法判断丢包或损坏", "提高传输可检测性"],
        ],
        [1.6, 2.6, 2.15],
    )

    add_heading(doc, "7 异常处理与排障", 1)
    add_table(
        doc,
        ["现象", "可能原因", "定位方法", "处理策略"],
        [
            ["波形频率错误", "TIM_CLK计算错误或混叠", "测量触发GPIO并换信号频率", "修正时钟并提高采样率"],
            ["双通道错位", "交错数据拆分错误", "两通道输入不同直流电平", "按ADC Rank顺序拆分"],
            ["波形随机断裂", "处理任务不及时或DMA覆盖", "查看Overrun和任务耗时", "降低负载或增大块尺寸"],
            ["触发位置漂移", "阈值噪声或边沿判断错误", "记录触发前后原始样本", "增加迟滞并检查状态机"],
            ["串口解析失败", "半包、粘包或CRC错误", "保存原始字节流", "接收缓冲加帧头重同步"],
            ["ADC幅值偏差", "参考电压、输入阻抗或保护电路", "用万用表和已知直流源校准", "增益和偏置校准"],
        ],
        [1.25, 1.75, 1.7, 2.25],
    )

    add_heading(doc, "8 测试与结果准备", 1)
    add_table(
        doc,
        ["测试项", "方法", "面试时要能给出的结果"],
        [
            ["采样率", "输出TIM触发测试点并用逻辑分析仪测量", "目标值、实际值和误差"],
            ["DMA处理裕量", "DWT或GPIO测半缓冲处理时间", "处理时间与覆盖时间的比例"],
            ["触发准确性", "输入已知方波并改变阈值", "触发位置误差和重复性"],
            ["幅值测量", "输入已知直流和正弦信号", "均值、峰峰值误差"],
            ["频率测量", "与信号源或标准示波器对比", "不同频率下的测量误差"],
            ["通信可靠性", "长时间上传并统计序号和CRC", "丢包率、CRC错误率和上传时间"],
        ],
        [1.3, 2.6, 2.75],
    )
    add_label_paragraph(doc, "结果记录模板  ", "最高稳定采样率____kHz；DMA块长____；窗口____点；处理裕量____%；触发误差____点；上传____KB耗时____s；CRC错误____次。")

    add_heading(doc, "9 高频面试问题与参考回答", 1)
    scope_qa = [
        ("为什么使用TIM触发ADC", "硬件触发能获得确定的采样间隔，避免软件调用、任务切换和中断延迟导致的不均匀采样。"),
        ("采样率如何计算", "根据定时器输入时钟、PSC和ARR计算，同时核对APB分频后的定时器倍频规则，并测量实际触发GPIO验证。"),
        ("ADC采样时间怎么选", "需要在信号源阻抗、精度和最大采样率之间平衡；两个通道总转换时间必须小于触发周期。"),
        ("为什么DMA比逐点ADC中断好", "DMA无需CPU参与逐点搬运，CPU只按半缓冲或整缓冲处理数据，显著减少中断次数和上下文切换。"),
        ("循环DMA和普通DMA有什么区别", "普通DMA到指定长度后停止；循环DMA到末尾会自动从头继续，适合连续采样，但必须防止CPU处理不及时导致覆盖。"),
        ("双缓冲如何避免竞争", "DMA和CPU在不同半区工作，中断只发布哪个半区可读；处理任务必须在该半区下次被覆盖前完成。"),
        ("DMA缓冲为什么不能随便放CCM", "F407的CCM通常只连接CPU总线，DMA无法访问，因此缓冲要放在DMA可访问SRAM，链接脚本也需确认。"),
        ("模拟看门狗能直接判断边沿吗", "它负责快速发现阈值事件，边沿类型仍要比较previous与current；为避免噪声反复触发可增加迟滞。"),
        ("如何实现前触发", "ARMED状态下持续写循环缓冲，触发时保存当前写指针，再采集后触发样本，最后从触发前位置按时间顺序整理窗口。"),
        ("为什么100kHz数据不能实时走115200串口", "双通道16位100kHz约400KB每秒，而115200 8N1有效仅约11.5KB每秒，因此必须采集后上传、抽点或更换高速接口。"),
        ("为什么串口一字节按10位计算", "8N1包含1位起始位、8位数据位和1位停止位，所以每个有效字节在线路上通常需要10位。"),
        ("CRC16有什么作用", "检测随机位错误和帧损坏，但没有密钥，不提供身份认证和防恶意篡改能力。"),
        ("如何处理半包和粘包", "Python维护持续接收缓冲，搜索帧头，等待长度字段对应的完整帧，校验CRC后消费；失败则移动到下一个可能帧头。"),
        ("如何计算频率", "基础方案用同方向过零点间隔计算周期；复杂波形可在上位机做FFT并根据峰值索引换算频率。"),
        ("什么是混叠", "输入频率超过奈奎斯特限制后会映射为错误的低频分量。解决办法是提高采样率，并在ADC前增加模拟低通滤波。"),
        ("12位和8位ADC如何取舍", "12位量化精度高但转换可能更慢、数据量仍按16位存储；8位可提升速度并降低有效数据量，需根据精度需求选择。"),
        ("处理任务来不及怎么办", "先确认平均算力是否不足，随后减少运算、增大DMA块、降低采样率或把复杂分析移到PC。单纯增大缓冲只能吸收短时波动。"),
        ("FreeRTOS是否决定采样精度", "不是。采样时刻由TIM、ADC和DMA硬件决定，FreeRTOS主要组织处理、控制、通信和监控任务。"),
        ("项目中最难的问题是什么", "高速采样率与低速通信接口之间的吞吐量矛盾。通过带宽计算后改成SRAM定长采集、分帧上传和抽点预览。"),
        ("你对开源项目做了什么", "按实际情况回答，例如移植到F407、加入FreeRTOS任务通知、双缓冲、前后触发状态机、CRC协议、带宽处理和Python上位机。"),
    ]
    add_table(doc, ["问题", "参考回答"], scope_qa, [2.05, 4.65])

    add_heading(doc, "10 一分钟速记清单", 1)
    add_bullets(
        doc,
        [
            "主线：配置 -> TIM3 TRGO -> ADC扫描 -> DMA双缓冲 -> 触发窗口 -> 分帧上传 -> Python。",
            "核心矛盾：100kHz双通道原始数据约400KB每秒，115200串口仅约11.5KB每秒。",
            "核心方案：硬件触发、DMA块处理、SRAM采集窗口、采集后上传和抽点预览。",
            "触发：模拟看门狗发现阈值，相邻样本判定边沿，循环缓冲保留前触发。",
            "可靠性：序号、长度、CRC16、Overrun计数、接收端半包和粘包处理。",
            "故障故事：发现串口带宽不够后，从实时全量传输改为定长采集后上传。",
        ],
    )
    add_shared_section(doc, "姿态采集与数据记录项目")
    add_40_minute_plan(doc, "scope")
    add_heading(doc, "13 参考项目", 1)
    add_bullets(
        doc,
        [
            "https://github.com/sentomarco/STM32-NUCLEO-oscilloscope",
            "https://github.com/ian-ross/stm32-timer-adc-dma",
        ],
    )
    doc.save(OUT_SCOPE)


build_imu_doc()
build_scope_doc()
print(OUT_IMU)
print(OUT_SCOPE)
