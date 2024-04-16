class MIPSAssembler:
    def __init__(self, filename):
        self.filename = filename
        self.instructions = []  
        self.labels = {}  # Store label names and their addresses
        self.cur_address = 0x0000  # Specified first address in coursework

    # Decoding inspired from:
    # https://github.com/kashyapakshay/PyMIPS/blob/master/Assembler.py
    def read_asm_file(self):
        with open(self.filename, 'r') as file:
            for line in file:
                stripped_line = line.strip()
                # Check if the line is a label
                if stripped_line.endswith(':'):
                    label_name = stripped_line[:-1]  # Remove the colon
                    self.labels[label_name] = self.cur_address
                elif stripped_line:  # Check if line is not empty
                    # Store the instruction and its address
                    self.instructions.append((self.cur_address, stripped_line))
                    self.cur_address += 4  

    def replace_labels_with_addresses(self):
        # Process each instruction to replace labels with addresses
        for index, (address, instruction) in enumerate(self.instructions):
            parts = instruction.split()
            # Replace last part of split line (label) with corresponding instruction address
            if parts[-1] in self.labels:
                # Replace the label with its address in hex format
                parts[-1] = f"0x{self.labels[parts[-1]]:04X}"
                # Reconstruct the instruction with the label replaced by address
                self.instructions[index] = (address, " ".join(parts))

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
    assembler.replace_labels_with_addresses() 
    assembler.display_instructions()
    assembler.display_labels()
