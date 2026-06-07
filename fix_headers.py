import os
import re
import datetime

HEADER_TEMPLATE = """/**
 * SPDX-FileComment: {component}
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file {filename}
 * @brief {component}
 * @version 0.2.0
 * @date {date}
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

"""

def process_file(path):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Remove existing header at the very beginning of the file if it exists
    content = re.sub(r'^\s*/\*\*.*?\*/\s*', '', content, flags=re.DOTALL)

    filename = os.path.basename(path)
    component = filename.split('.')[0]
    date_str = "2026-06-07"

    new_header = HEADER_TEMPLATE.format(component=component, filename=filename, date=date_str)
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(new_header + content.lstrip())

for root, _, files in os.walk('src'):
    for file in files:
        if file.endswith('.cpp') or file.endswith('.h'):
            process_file(os.path.join(root, file))

for root, _, files in os.walk('configure'):
    for file in files:
        if file.endswith('.hpp.in'):
            process_file(os.path.join(root, file))

print("Headers updated.")
