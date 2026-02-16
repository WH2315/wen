import sys
import os
import re

def is_header(filename):
    return filename.endswith(".hpp") or filename.endswith(".h")

class RemoveComments:
    def __init__(self, f):
        self.lines = []
        self.line_number = 0

        lines = f.readlines()
        self.need_parse = "REFLECT_CLASS" in "".join(lines)
        if not self.need_parse:
            return

        in_str = False
        fragment_comment = False
        for line in lines:
            new_line = ""
            for i, c in enumerate(line):
                if c == '"':
                    in_str = not in_str
                if not in_str:
                    if c == '/' and i + 1 < len(line):
                        if line[i + 1] == '/':
                            break
                        if line[i + 1] == '*':
                            fragment_comment = True
                if not fragment_comment:
                    new_line += c
                else:
                    if c == '/' and i >= 1 and line[i - 1] == '*':
                        fragment_comment = False
            new_line = new_line.strip("\n").rstrip(" ")
            if new_line:
                self.lines.append(new_line + "\n")

    def readline(self):
        if self.line_number == len(self.lines):
            return ""
        self.line_number += 1
        return self.lines[self.line_number - 1]


class ClassInfo:
    def __init__(self, name, qualified_name, header):
        self.name = name
        self.qualified_name = qualified_name
        self.header = header
        self.reflect_name = None
        self.members = []
        self.functions = []


class ClassContext:
    def __init__(self, info, depth):
        self.info = info
        self.depth = depth
        self.pending_member = None
        self.pending_function = None


def parse_macro_args(line, macro):
    pattern = r"%s\s*\(([^)]*)\)" % re.escape(macro)
    match = re.search(pattern, line)
    if not match:
        return []
    raw = match.group(1).strip()
    if not raw:
        return []
    args = [item.strip() for item in raw.split(",") if item.strip()]
    return args


def normalize_name_arg(arg):
    if arg.startswith('"') and arg.endswith('"') and len(arg) >= 2:
        return arg[1:-1]
    return arg


def strip_literals(line):
    result = []
    in_str = False
    in_char = False
    escape = False
    for c in line:
        if escape:
            escape = False
            result.append(" ")
            continue
        if c == "\\":
            escape = True
            result.append(" ")
            continue
        if in_str:
            if c == '"':
                in_str = False
            result.append(" ")
            continue
        if in_char:
            if c == "'":
                in_char = False
            result.append(" ")
            continue
        if c == '"':
            in_str = True
            result.append(" ")
            continue
        if c == "'":
            in_char = True
            result.append(" ")
            continue
        result.append(c)
    return "".join(result)


def extract_member_name(line):
    if "(" in line:
        return None
    sanitized = strip_literals(line)
    sanitized = re.sub(r"\b\d+(?:\.\d+)?(?:[eE][+-]?\d+)?[fFlLuU]*\b", " ", sanitized)
    identifiers = re.findall(r"[A-Za-z_]\w*", sanitized)
    if not identifiers:
        return None
    return identifiers[-1]


def extract_function_name(line):
    match = re.search(r"([A-Za-z_]\w*)\s*\(", line)
    if not match:
        return None
    name = match.group(1)
    if name in {"if", "for", "while", "switch", "return"}:
        return None
    return name


def remove_macro_from_line(line, macro):
    pattern = r"%s\s*\([^)]*\)" % re.escape(macro)
    return re.sub(pattern, "", line)


def parse_header(path, include_root):
    classes = []
    with open(path, "r", encoding="utf-8") as f:
        reader = RemoveComments(f)
        if not reader.need_parse:
            return classes
        lines = reader.lines

    depth = 0
    pending_class_name = None
    namespace_stack = []
    class_stack = []

    for raw_line in lines:
        line = raw_line.strip()

        open_count = raw_line.count("{")
        close_count = raw_line.count("}")

        namespace_matches = re.findall(r"\bnamespace\s+([A-Za-z_]\w*(?:::\w+)*)\s*\{", raw_line)
        for name in namespace_matches:
            namespace_stack.append((name, depth + 1))

        class_match = re.search(r"^\s*(class|struct)\s+([A-Za-z_]\w*)\b", raw_line)
        if class_match:
            name = class_match.group(2)
            is_forward = (";" in raw_line) and ("{" not in raw_line)
            if not is_forward:
                if "{" in raw_line:
                    qualified = "::".join([ns for ns, _ in namespace_stack] + [name])
                    info = ClassInfo(name, qualified, include_root)
                    class_stack.append(ClassContext(info, depth + 1))
                    classes.append(info)
                else:
                    pending_class_name = name

        if pending_class_name and open_count > 0:
            qualified = "::".join([ns for ns, _ in namespace_stack] + [pending_class_name])
            info = ClassInfo(pending_class_name, qualified, include_root)
            class_stack.append(ClassContext(info, depth + 1))
            classes.append(info)
            pending_class_name = None

        if class_stack:
            context = class_stack[-1]
            if "REFLECT_CLASS" in raw_line:
                args = parse_macro_args(raw_line, "REFLECT_CLASS")
                if args:
                    context.info.reflect_name = normalize_name_arg(args[0])
                else:
                    context.info.reflect_name = context.info.qualified_name

            if "REFLECT_MEMBER" in raw_line:
                args = parse_macro_args(raw_line, "REFLECT_MEMBER")
                explicit_name = normalize_name_arg(args[0]) if args else None
                candidate_line = remove_macro_from_line(raw_line, "REFLECT_MEMBER")
                member_name = extract_member_name(candidate_line)
                if member_name:
                    reflect_name = explicit_name or member_name
                    context.info.members.append((reflect_name, member_name))
                else:
                    context.pending_member = explicit_name or ""

            if "REFLECT_FUNCTION" in raw_line:
                args = parse_macro_args(raw_line, "REFLECT_FUNCTION")
                explicit_name = normalize_name_arg(args[0]) if args else None
                candidate_line = remove_macro_from_line(raw_line, "REFLECT_FUNCTION")
                function_name = extract_function_name(candidate_line)
                if function_name:
                    reflect_name = explicit_name or function_name
                    context.info.functions.append((reflect_name, function_name))
                else:
                    context.pending_function = explicit_name or ""

            if context.pending_member is not None and "REFLECT_MEMBER" not in raw_line:
                if line and not line.startswith("#") and not line.startswith("public") \
                    and not line.startswith("private") and not line.startswith("protected"):
                    member_name = extract_member_name(raw_line)
                    if member_name:
                        reflect_name = context.pending_member or member_name
                        context.info.members.append((reflect_name, member_name))
                        context.pending_member = None

            if context.pending_function is not None and "REFLECT_FUNCTION" not in raw_line:
                if line and not line.startswith("#") and not line.startswith("public") \
                    and not line.startswith("private") and not line.startswith("protected"):
                    function_name = extract_function_name(raw_line)
                    if function_name:
                        reflect_name = context.pending_function or function_name
                        context.info.functions.append((reflect_name, function_name))
                        context.pending_function = None

        depth += open_count - close_count

        while namespace_stack and depth < namespace_stack[-1][1]:
            namespace_stack.pop()

        while class_stack and depth < class_stack[-1].depth:
            class_stack.pop()

    return classes


def collect_headers(root_dirs):
    headers = []
    for root in root_dirs:
        if not os.path.isdir(root):
            continue
        for base, _, files in os.walk(root):
            for name in files:
                if is_header(name):
                    headers.append(os.path.join(base, name))
    return headers


def to_include_path(header_path, include_roots):
    normalized = header_path.replace("\\", "/")
    for root in include_roots:
        root_norm = root.replace("\\", "/")
        if normalized.startswith(root_norm + "/"):
            return normalized[len(root_norm) + 1 :]
    return normalized


def generate_cpp(classes, include_roots, output_path):
    includes = []
    for cls in classes:
        if cls.reflect_name is None:
            continue
        include_path = to_include_path(cls.header, include_roots)
        if include_path not in includes:
            includes.append(include_path)

    lines = []
    lines.append("// This file is auto-generated by engine/parser/parser.py.\n")
    lines.append("// Do not edit manually.\n\n")
    lines.append('#include "engine/global_context.hpp"\n')
    lines.append('#include "core/reflect/reflect_system.hpp"\n')
    for inc in includes:
        lines.append(f'#include "{inc}"\n')
    lines.append("\n")
    lines.append("void Parser() {\n")
    lines.append("    using namespace wen;\n")
    lines.append("    if (global_context == nullptr) {\n")
    lines.append("        return;\n")
    lines.append("    }\n")
    lines.append("    if (global_context->reflect_system.getInstance() == nullptr) {\n")
    lines.append("        return;\n")
    lines.append("    }\n")
    lines.append("    auto& reflect_system = *global_context->reflect_system;\n\n")

    for cls in classes:
        if cls.reflect_name is None:
            continue
        lines.append("    {\n")
        lines.append(
            f"        auto class_builder = reflect_system.addClass<{cls.qualified_name}>(\"{cls.reflect_name}\");\n"
        )
        for reflect_name, member_name in cls.members:
            lines.append(
                f"        class_builder.addMember(\"{reflect_name}\", &{cls.qualified_name}::{member_name});\n"
            )
        for reflect_name, function_name in cls.functions:
            lines.append(
                f"        class_builder.addFunction(\"{reflect_name}\", &{cls.qualified_name}::{function_name});\n"
            )
        lines.append("    }\n\n")

    lines.append("}\n")

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        f.writelines(lines)


def parse_args(argv):
    roots = []
    output = None
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg in {"-r", "--root"} and i + 1 < len(argv):
            roots.append(argv[i + 1])
            i += 2
        elif arg in {"-o", "--out"} and i + 1 < len(argv):
            output = argv[i + 1]
            i += 2
        else:
            i += 1
    return roots, output


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    engine_dir = os.path.abspath(os.path.join(script_dir, ".."))
    default_roots = [
        os.path.join(engine_dir, "runtime", "include"),
        os.path.join(engine_dir, "editor", "include"),
    ]

    output_dir = sys.argv[1]
    input_dir = sys.argv[2]
    search_roots = [input_dir]
    headers = collect_headers(search_roots)
    all_classes = []
    for header in headers:
        classes = parse_header(header, header)
        all_classes.extend(classes)
    output_path = os.path.join(output_dir, "auto_generated.cpp")

    generate_cpp(all_classes, search_roots, output_path)


if __name__ == "__main__":
    main()
