**Mav Shell (msh)** is a custom Unix-like shell written in C, developed to explore and demonstrate core systems programming concepts such as process creation, I/O redirection, command piping, and command history management. Inspired by shells like Bash and Csh, it provides a lightweight command-line interface with essential features.

---

## 🚀 Features

- **Custom Prompt:** Displays `msh>` when ready for input
- **Command Execution:** Runs standard Linux system commands
- **Built-in Commands:**  
  - `cd` – change directories  
  - `exit` / `quit` – exit the shell gracefully  
- **Input/Output Redirection:**  
  - Use `>` to redirect output to a file  
  - Use `<` to take input from a file  
- **Piping:** Supports connecting multiple commands with `|`
- **Command History:**
  - Stores the last 50 commands
  - View history with `show history`
  - Re-execute previous commands using `!command_name`
- **Robust Parsing:** Handles up to 10 command-line arguments
- **Handles Blank Input:** Simply reprints the prompt with no error

---

## 🛠️ Compilation & Running

```bash
gcc msh.c -o msh
./msh
