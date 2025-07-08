# Dash-CPP

Dash-CPP is a C++ reimplementation of the dash shell, aiming to provide a modern, object-oriented shell implementation.

## Features

- Utilizes modern C++17 features
- Object-oriented design
- Exception handling mechanism
- Smart pointers for memory management
- Modular architecture
- Unit testing support

## Current Status

The project is currently under active development. The following components have been implemented:

- Core shell infrastructure
- Input handling system
- Variable management
- Lexical analyzer
- Parser for shell commands
- Abstract syntax tree nodes
- Command execution framework
- Job control system
- Basic built-in commands (cd, exit, echo)

## Build Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or higher
- Optional: Google Test (for unit testing)

## Build Steps

```bash
# Clone the repository
git clone https://github.com/yourusername/dash-cpp.git
cd dash-cpp

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests (optional)
ctest
```

## Project Structure

```
dash-cpp/
├── include/                 # Header files
│   ├── builtins/            # Built-in commands
│   ├── core/                # Core components
│   ├── job/                 # Job control
│   ├── utils/               # Utility classes
│   ├── variable/            # Variable management
│   └── dash.h               # Main header file
├── src/                     # Source files
│   ├── builtins/            # Built-in command implementations
│   ├── core/                # Core component implementations
│   ├── job/                 # Job control implementations
│   ├── utils/               # Utility class implementations
│   ├── variable/            # Variable management implementations
│   └── main.cpp             # Program entry point
├── tests/                   # Test files
├── CMakeLists.txt           # CMake configuration file
└── README.md                # This file
```

## Major Components

- **Shell**: The core controller of the program, coordinating the work of other components
- **Parser**: Parses command line input and builds an abstract syntax tree
- **Lexer**: Lexical analyzer, breaking input into lexical tokens
- **Executor**: Executes command trees
- **InputHandler**: Handles input sources (standard input, files, strings)
- **VariableManager**: Manages shell variables and environment variables
- **JobControl**: Manages job control
- **Node**: Abstract syntax tree nodes

## Roadmap

- Implement more built-in commands
- Enhance job control functionality
- Add command history and line editing
- Implement command completion
- Add scripting capabilities (loops, conditionals, functions)
- Improve error handling and reporting

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing

Contributions to the code, reporting issues, or suggesting improvements are welcome!
