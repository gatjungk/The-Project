# Understanding of Strip from:
# https://www.geeksforgeeks.org/python-string-strip/

# Splitting instruction line and class structure inspired from:
# https://github.com/kashyapakshay/PyMIPS/blob/master/Assembler.py

# Use of read and write to a function from:
# https://www.freecodecamp.org/news/python-write-to-file-open-read-append-and-other-file-handling-functions-explained/


import struct

# Encode R-type instructions
def encode_r_type(rs, rt, rd, shamt, funct):
    instruction = (0 << 26) | (rs << 21) | (rt << 16) | (rd << 11) | (shamt << 6) | funct
    return instruction

# Encode I-type instructions
def encode_i_type(opcode, rs, rt, immediate):
    return (opcode << 26) | (rs << 21) | (rt << 16) | (immediate & 0xFFFF)

# Encode J-type instructions
def encode_j_type(opcode, address):
    return (opcode << 26) | (address & 0x3FFFFFF)

# Maps instruction names to their funct codes
funct_codes = {
    'ADD': 0x20,
    'SUB': 0x22,
    'AND': 0x24,
    'OR': 0x25,
    'XOR': 0x26,
    'NOR': 0x27,
    'SLLV': 0x04,
    'SRLV': 0x06,
    'JR': 0x08,
    'SLT': 0x2a, 
}

# Maps instruction names to their opcodes
operation_code = {
    #R-type Instructions 
    'ADD': 0x00,
    'SUB': 0x00,
    'AND': 0x00,
    'OR': 0x00,
    'XOR': 0x00,
    'NOR': 0x00, 
    'SLLV': 0x00,
    'SRLV': 0x00,
    'JR': 0x00,
    'SLT': 0x00, 
    #I-type Instructions 
    "ADDI": 0x08,
    'BEQ': 0x04, 
    'BNE': 0x05, 
    'SW': 0x2b,
    'LW': 0x23, 
    #J-type Instructions
    'J': 0x02
}

# Maps register names to their decimal register numbers 
register_map = {
    # Reg Name
    '$zero': 0, '$at': 1, 
    '$v0': 2, '$v1': 3,
    '$a0': 4, '$a1': 5, '$a2': 6, '$a3': 7,
    '$t0': 8, '$t1': 9, '$t2': 10, '$t3': 11, 
    '$t4': 12, '$t5': 13, '$t6': 14, '$t7': 15,
    '$s0': 16, '$s1': 17, '$s2': 18, '$s3': 19,
    '$s4': 20, '$s5': 21, '$s6': 22, '$s7': 23,
    '$t8': 24, '$t9': 25, 
    '$k0': 26, '$k1': 27,
    '$gp': 28, '$sp': 29, 
    '$fp': 30, '$ra': 31,

    # Reg Number 
    '$0': 0, '$1': 1, 
    '$2': 2, '$3': 3,
    '$4': 4, '$5': 5, '$6': 6, '$7': 7,
    '$8': 8, '$9': 9, 
    '$10': 10, '$11': 11, 
    '$12': 12, '$13': 13, '$14': 14, '$15': 15,
    '$16': 16, '$17': 17, '$18': 18, '$19': 19,
    '$20': 20, '$21': 21, '$22': 22, '$23': 23,
    '$24': 24, '$25': 25, 
    '$26': 26, '$27': 27,
    '$28': 28, '$29': 29, 
    '$30': 30, '$31': 31
}

class MIPSAssembler:
    def __init__(self, filename):
        self.filename = filename
        self.instructions = [] 
        self.labels = {} # Store label names and their addresses
        self.cur_address = 0x0000 # Specified first address in coursework

    def read_asm_file(self):
        with open(self.filename, 'r') as file:
            for line in file:
                stripped_line = line.strip()
                # Check if the line is a label
                if stripped_line.endswith(':'):
                    label_name = stripped_line[:-1]
                    self.labels[label_name] = self.cur_address
                # Check if line is not empty
                elif stripped_line:
                    # Store the instruction and its address
                    self.instructions.append((self.cur_address, stripped_line))
                    self.cur_address += 4

    def replace_labels_with_addresses(self):
        # Unpacks the self.instruction into its instruction and its addresses
        for index, (address, instruction) in enumerate(self.instructions):
            parts = instruction.split()
            for part_index, part in enumerate(parts):
                if part in self.labels:
                    # Replace the label with its decimal word address
                    word_address = self.labels[part] // 4
                    parts[part_index] = str(word_address)
            # Reconstruct the instruction with the label replaced by address
            self.instructions[index] = (address, " ".join(parts))

    def encode_instructions(self):
            encoded_instructions = []
            for address, instruction in self.instructions:
                parts = instruction.split()
                operation = parts[0]

                # R-type instructions with special handling for SLLV, SRLV, JR instructions
                if operation in ['ADD', 'SUB', 'AND', 'OR','XOR','NOR','SLLV', 'SRLV', 'JR', 'SLT']:
                    funct = funct_codes[operation]
                    rd = register_map[parts[1].strip(',')]
                    if operation in ['SLLV', 'SRLV']:
                        rt = register_map[parts[2].strip(',')]
                        rs = register_map[parts[3].strip(',')]
                    elif operation in ['JR']:
                        rs = register_map[parts[1].strip(',')]
                        rt = 0
                        rd = 0
                    else:
                        rs = register_map[parts[2].strip(',')]
                        rt = register_map[parts[3].strip(',')]
                    shamt = 0  # Shift Operator not used
                    opcode = operation_code[operation]
                    encoded = encode_r_type(rs, rt, rd, shamt, funct)
                    encoded_instructions.append(encoded)

                # I-type instructions with special handling for LW, SW, BNE, BEQ instructions
                elif operation in ['ADDI', 'LW', 'SW', 'BNE', 'BEQ']:
                    opcode = operation_code[operation]
                    rt = register_map[parts[1].strip(',')]
                    if operation in ['LW', 'SW']:
                        offset_str, rs_str = parts[2][:-1].split('(')  # Extract offset and rs
                        offset = int(offset_str) if offset_str else 0  
                        rs = register_map[rs_str]
                    elif operation in ['BNE', 'BEQ']:
                        #Swap rt and rs positions
                        rs = register_map[parts[1].strip(',')]
                        rt = register_map[parts[2].strip(',')]
                        # Already in word address from label replacement
                        label_address = int(parts[3])
                        # Make imm relative addressed 
                        offset = label_address - ((address+4))//4 
                    else:
                        rs = register_map[parts[2].strip(',')]
                        offset = int(parts[3])  # Immediate value
                    encoded = encode_i_type(opcode, rs, rt, offset)
                    encoded_instructions.append(encoded)

                # J-type instruction
                elif operation in ['J']:
                    opcode = operation_code[operation]
                    jaddr = int(parts[1])
                    encoded = encode_j_type(opcode, jaddr)
                    encoded_instructions.append(encoded)

            return encoded_instructions
    
    def write_to_binary_file(self, output_filename):
        encoded_instructions = self.encode_instructions()
        with open(output_filename, 'wb') as file:
            for instruction in encoded_instructions:
                # Writing bytes in little-endian
                file.write(struct.pack('<I', instruction)) 

    def print_instructions_as_hex(self):
        encoded_instructions = self.encode_instructions()
        for instruction in encoded_instructions:
            # Convert to hexadecimal and print
            print("0x" + format(instruction, '08X'))

if __name__ == "__main__":
    assembler = MIPSAssembler('example_A1.asm')  
    assembler.read_asm_file()
    assembler.replace_labels_with_addresses()
    assembler.print_instructions_as_hex() 
    assembler.write_to_binary_file('output.bin')  
    print("Assembly completed.")
