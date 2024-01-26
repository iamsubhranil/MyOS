
SYMNAMES = {
    '`': "Backtick",
    ',': "Comma",
    '.': "Dot",
    '/': "Backslash",
    "'": "SingleQuote",
    '[': "SquareOpen",
    ']': "SquareClose",
    '=': "Equals",
    '\\': "ForwardSlash",
    '+': "Plus",
    '-': "Minus",
    '*': "Star",
    ';': "Semicolon",
}

SYMBOLMAP = {v.capitalize(): k for k, v in SYMNAMES.items()}

MODIFIERS = ["Left_Control", "Right_Control", "Left_Alt", "Right_Alt", "Numberlock", "Left_Shift", "Right_Shift", "Capslock"]

HASASCII = {"Space": ' ', "Tab": "\\t", "Backspace": "\\b", "Enter": "\\n"}

SHIFTMAPPING = {
    "Backtick": "~", "1": "!", "2": "@", "3": "#", "4": "$", "5": "%", "6": "^", "7": "&",
    "8": "*", "9": "(", "0": ")", "Squareopen": "{", "Squareclose": "}", "Semicolon": ":",
    "Forwardslash": "|", "Singlequote": "\"", "Comma": "<", "Dot": ">", "Backslash": "?",
    "Minus": "_", "Equals": "+"
}

class SourceCode:

    def __init__(self):
        self.source = ""
        self.tabs = ""

    def indent(self):
        self.tabs += "\t"

    def dedent(self):
        if len(self.tabs) == 1:
            self.tabs = ""
        else:
            self.tabs = self.tabs[:-1]

    def print(self, *values, **kwargs):
        if values[-1][0] == '}':
            self.dedent()
        addstr = "".join(values)
        if len(addstr) < 80 and len(self.source.split("\n")[-1]) + len(addstr) > 80:
            self.source += '\n'
        if self.source.endswith('\n'):
            self.source += self.tabs
        self.source += addstr
        if "end" in kwargs:
            self.source += kwargs["end"]
        else:
            self.source += "\n"
            if values[-1][-1] == '{':
                self.indent()

    def print_lines(self, *lines):
        for l in lines:
            self.print(l)

    def __repr__(self):
        return self.source

    def __str__(self):
        return self.source

def generate_states(levelStates, level, enumNames, source):
    source.print("bool handleKeyLevel" + str(level) + "(u8 byte) {")

    later = []
    before_if = False
    for s in levelStates:
        prevcode = s[0]
        states = s[1]
        if prevcode != None:
            source.print("else " if before_if else "", "if(lastLevelCode == ", prevcode + ") {")
            before_if = True
        source.print("switch(byte) {")
        for k in states:
            if type(states[k]) == str:
                enum = enumNames[states[k]]
                if states[k].endswith("pressed"):
                    source.print("case ", k + ": {")
                    source.print("keys.put(Key::" + enum + ");")
                    if enum in MODIFIERS:
                        source.print("modifierPressed(Key::" + enum + ");")
                    source.print("return true;")
                    source.print("}")
                else:
                    # released
                    if enum in MODIFIERS:
                        source.print("case ", k + ": {")
                        source.print("// ", enum, " released")
                        source.print("modifierReleased(Key::" + enum + ");")
                        source.print("return false;")
                        source.print("}")
            else:
                source.print("case ", k + ": {")
                source.print("lastLevelCode = ", k, ";")
                source.print("state = KeyLevel" + str(level + 1) + ";")
                source.print("return false;")
                later.append((k, states[k]))
                source.print("}")
        source.print("default: {")
        source.print("state = KeyLevel0;")
        source.print("return false;")
        source.print("}")
        source.print("}")
        if prevcode != None:
            source.print("}")

    source.print("return false;")
    source.print("}")
    source.print("\n")
    if len(later) > 0:
        return generate_states(later, level + 1, enumNames, source)
    else:
        return level

def main(input_, output):
    lines = None
    with open(input_, "r") as f:
        lines = f.readlines()
    if lines == None:
        return
    keys = {}
    for line in lines:
        parts = line.strip().split("\t")
        # print(parts)
        codes = parts[0]
        key = parts[1]
        keys[codes] = key

    # print(keys)

    key_vars = {}
    for k in keys.values():
        key = k.replace(" pressed", "").replace(" released", "")
        key = key.replace("(", "").replace(")", "")
        parts = key.split(" ")
        if len(parts[-1]) == 1:
            if parts[-1].isalpha():
                parts = ["Alpha", *parts]
            elif parts[-1].isnumeric():
                parts = ["Num", *parts]
            else:
                parts = ["Sym", *parts[:-1], SYMNAMES[parts[-1]]]
        key = "_".join([p.capitalize() for p in parts])
        # key = key.capitalize()
        key_vars[k] = key

    all_keys = set(key_vars.values())
    all_keys = list(sorted(all_keys))

    source = SourceCode()

    source.print("#include <ds/staticqueue.h>\n")

    source.print("enum Key : u8 {")

    for i, k in enumerate(all_keys):
        source.print(k + " = " + str(i), end=", ")

    source.print("};")

    states = {}
    for k in keys:
        codes = k.split(",")
        if len(codes) == 1:
            states[k] = keys[k]
        else:
            if codes[0] not in states:
                states[codes[0]] = {}
            curdict = states[codes[0]]
            for c in codes[1:-1]:
                if c not in curdict:
                    curdict[c] = {}
                curdict = curdict[c]
            curdict[codes[-1]] = keys[k]
    print(states)

    source.print("\n")

    source.print("struct ScancodeHandler {")

    source.print("StaticQueue<Key, 1024> keys;")
    source.print("u64 modifierStates;")
    source.print("u8 lastLevelCode;")
    source.print("ScancodeHandler() : keys(), modifierStates(0), lastLevelCode(0), state(KeyLevel0)  {}")

    source.print("int getModifierIndex(Key modifier) {")
    source.print("switch(modifier) {")
    for i, m in enumerate(MODIFIERS):
        source.print("case Key::" + m + ": return " + str(i + 1) + ";")
    source.print("default: return 0;")
    source.print("}")
    source.print("}")

    source.print("void modifierPressed(Key modifier) {")
    source.print("int idx = getModifierIndex(modifier);")
    source.print("if(idx == 0) return;")
    source.print("modifierStates |= ((u64)1 << idx);")
    source.print("}")

    source.print("void modifierReleased(Key modifier) {")
    source.print("int idx = getModifierIndex(modifier);")
    source.print("modifierStates &= ~((u64)1 << idx);")
    source.print("}")

    source.print("bool isModifierPressed(Key modifier) {")
    source.print("int idx = getModifierIndex(modifier);")
    source.print("return (modifierStates & ((u64)1 << idx)) != 0;")
    source.print("}")

    alreadyChecked = []
    for m in MODIFIERS:
        p = m.split("_")
        if p[0] in ["Left", "Right"]:
            if p[1] not in alreadyChecked:
                source.print("bool is", p[1], "Pressed() {")
                source.print("return isModifierPressed(Key::Left_", p[1], ") || isModifierPressed(Key::Right_", p[1], ");")
                source.print("}")
                alreadyChecked.append(p[1])
        else:
            source.print("bool is", p[0], "Pressed() {")
            source.print("return isModifierPressed(Key::", p[0], ");")
            source.print("}")

    maxLevel = generate_states([(None, states)], 0, key_vars, source)

    source.print("char getNextASCII() {")
    source.print("if(keys.size() == 0) return 0;")
    source.print("Key k = keys.get();")
    source.print("char keyMapping[] = {")

    for k in all_keys:
        if k.startswith("Alpha"):
            ret = k.split("_")[-1].lower()
            source.print("'" + ret + "'", end=", ")
        elif k.startswith("Num_"):
            source.print("'" + k.split("_")[-1] + "'", end=", ")
        elif k.split("_")[-1] in HASASCII.keys():
            source.print("'" + HASASCII[k.split("_")[-1]] + "'", end=", ")
        elif k.startswith("Sym_"):
            name = k.split("_")[-1]
            ret = SYMBOLMAP[name]
            if ret == "\\":
                ret = "\\\\"
            elif ret == "'":
                ret = "\\'"
            source.print("'", ret, "'", end=", ")
        else:
            source.print("0", end=", ")
    source.print("};")
    source.print("char res = keyMapping[k];")
    source.print("if(isCapslockPressed() && res >= 'a' && res <= 'z' && !isShiftPressed()) {")
    source.print("res = res - 32;")
    source.print("}")
    source.print("if(res != 0 && isShiftPressed()) {")
    source.print("char shiftTable[] = {")
    for k in all_keys:
        lastpart = k.split("_")[-1]
        if k.startswith("Alpha"):
            ret = lastpart.upper()
            source.print("'" + ret + "'", end=", ")
        elif lastpart in SHIFTMAPPING:
            ret = SHIFTMAPPING[lastpart]
            source.print("'", ret, "'", end=", ")
        else:
            source.print("0", end=", ")
    source.print("};")
    source.print("if (shiftTable[k] != 0) {")
    source.print("res = shiftTable[k];")
    source.print("}")
    source.print("}")

    source.print("return res;")
    source.print("}")

    source.print("enum KeyLevel {")
    for i in range(maxLevel + 1):
        source.print("KeyLevel" + str(i), end=", ")
    source.print("};")

    source.print("KeyLevel state;")

    source.print("bool handleKey(u8 key) {")
    source.print("switch(state) {")
    for i in range(maxLevel + 1):
        source.print("case KeyLevel" + str(i), ": return handleKeyLevel" + str(i) + "(key);")
    source.print("}")
    source.print("return false;")
    source.print("}")

    source.print("};")

    print(source)

    with open(output, "w") as f:
        f.write(str(source))

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 3:
        print("Usage:", sys.argv[0], " <input_scantable> <output_header>")
    else:
        main(sys.argv[1], sys.argv[2])
