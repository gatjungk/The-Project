class MIPSAssembler:
    def __init__(self, filename):
        self.filename = filename
        self.instructions = []  
        self.labels = {}  # Store label names and their addresses
        self.cur_address = 0x0000

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

    # Debugging Output of Instructions with address
    def display_instructions(self):
        for address, instruction in self.instructions:
            print(f"0x{address:04X}: {instruction}")

    # Debugging Output of lables with address
    def display_labels(self):
        for label, address in self.labels.items():
            print(f"{label}: 0x{address:04X}")

if __name__ == "__main__":
    assembler = MIPSAssembler('example.asm')  
    assembler.read_asm_file()
    assembler.display_instructions()
    assembler.display_labels()