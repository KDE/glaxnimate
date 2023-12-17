#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: GPL-3.0-or-later

import argparse
import pathlib
from xml.etree import ElementTree


def convert_actions(in_menu: ElementTree.Element, out_menu: ElementTree.Element, sub_menus):
    for in_action in in_menu.findall("./addaction"):
        name = in_action.attrib["name"]
        sub_menu = sub_menus.get(name, None)
        if sub_menu is not None:
            out_action = ElementTree.SubElement(out_menu, "Menu")
            out_action.attrib["name"] = name
            convert_menu(sub_menu, out_action)
        elif name == "separator":
            ElementTree.SubElement(out_menu, "Separator")
        else:
            ElementTree.SubElement(out_menu, "Action").attrib["name"] = name


def convert_menu(in_menu: ElementTree.Element, out_menu: ElementTree.Element):
    if out_menu.tag == "Menu":
        title = in_menu.find("./property[@name='title']/string")
        ElementTree.SubElement(out_menu, "text").text = title.text

    out_menu.attrib["name"] = in_menu.attrib["name"]
    sub_menus = {
        menu.attrib["name"]: menu
        for menu in in_menu.findall("./widget/[@class='QMenu']")
    }
    convert_actions(in_menu, out_menu, sub_menus)


def convert_toolbar(in_toolbar: ElementTree.Element, out_toolbar: ElementTree.Element):
    title = in_toolbar.find("./property[@name='windowTitle']/string")
    ElementTree.SubElement(out_toolbar, "text").text = title.text

    out_toolbar.attrib["name"] = in_toolbar.attrib["name"]

    button_style = in_toolbar.find("./property[@name='toolButtonStyle']/string")
    if button_style:
        icon_text = {
            "ToolButtonIconOnly": "icononly",
            "ToolButtonTextOnly": "textonly",
            "ToolButtonTextBesideIcon": "icontextright",
            "ToolButtonTextUnderIcon": "icontextbottom",
            "ToolButtonFollowStyle": None,
        }[button_style.text]
    else:
        icon_text = "icononly"
    if icon_text:
        out_toolbar.attrib["iconText"] = icon_text

    out_toolbar.attrib["fullWidth"] = "true"
    out_toolbar.attrib["newline"] = in_toolbar.find("./attribute[@name='toolBarBreak']/bool").text

    area = in_toolbar.find("./attribute[@name='toolBarArea']/enum").text.replace("ToolBarArea", "").lower()
    out_toolbar.attrib["position"] = area
    convert_actions(in_toolbar, out_toolbar, {})


parser = argparse.ArgumentParser(
    description="Convert Qt Designer ui files to KXmlGui rc files for menus and toolbars"
)
parser.add_argument(
    "input",
    type=pathlib.Path,
    help="Input file (.ui)"
)
parser.add_argument(
    "--output", "-o",
    default="-",
    help="Output file, defaults to stdout"
)

args = parser.parse_args()
version = "1"
name = ""

if args.output != "-":
    outpath = pathlib.Path(args.output)
    if outpath.exists():
        existing = ElementTree.parse(outpath).getroot()
        version = str(int(existing.attrib["version"]) + 1)
        name = existing.attrib["name"]
    else:
        name = outpath.stem
        if name.endswith("ui"):
            name = name[:-2]


input_tree = ElementTree.parse(args.input)

output_tree = ElementTree.Element("gui")
output_tree.attrib["name"] = name
output_tree.attrib["version"] = version
output_tree.attrib["xmlns"] = "http://www.kde.org/standards/kxmlgui/1.0"
output_tree.attrib["xmlns:xsi"] = "http://www.w3.org/2001/XMLSchema-instance"
output_tree.attrib["xsi:schemaLocation"] = "http://www.kde.org/standards/kxmlgui/1.0/kxmlgui.xsd"

menubar = input_tree.find("./widget/widget[@class='QMenuBar']")
convert_menu(menubar, ElementTree.SubElement(output_tree, "MenuBar"))

for in_toolbar in input_tree.findall("./widget/widget[@class='QToolBar']"):
    convert_toolbar(in_toolbar, ElementTree.SubElement(output_tree, "ToolBar"))

ElementTree.indent(output_tree)

if args.output == "-":
    print(ElementTree.tostring(output_tree, "unicode", xml_declaration=True))
else:
    with open(args.output, "wb") as file:
        file.write(ElementTree.tostring(output_tree, "utf-8", xml_declaration=True))
