# Contributing to SHM-PCC-SDK

Thank you for your interest in contributing to SHM-PCC-SDK! This document provides guidelines and instructions for contributing.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Documentation](#documentation)
- [Submitting Changes](#submitting-changes)
- [Review Process](#review-process)

## Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inclusive environment for all contributors.

### Expected Behavior

- Be respectful and considerate
- Welcome newcomers and help them learn
- Focus on constructive feedback
- Respect different viewpoints and experiences

### Unacceptable Behavior

- Harassment or discrimination
- Trolling or insulting comments
- Personal attacks
- Any other unprofessional conduct

## Getting Started

### Prerequisites

- Git
- C++17 compatible compiler
- CMake 3.10+
- Basic understanding of C++ and concurrent programming

### Setting Up Development Environment

1. **Fork the Repository**
   ```bash
   # Fork on GitHub, then clone your fork
   git clone https://github.com/your-username/shm-pcc-sdk.git
   cd shm-pcc-sdk
   ```

2. **Add Upstream Remote**
   ```bash
   git remote add upstream https://github.com/original-org/shm-pcc-sdk.git
   ```

3. **Create Development Branch**
   ```bash
   git checkout -b dev
   ```

4. **Build the Project**
   ```bash
   cd shm-lib
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

## Development Workflow

### Branch Naming

Use descriptive branch names:
- `feature/description` - New features
- `fix/description` - Bug fixes
- `docs/description` - Documentation updates
- `refactor/description` - Code refactoring
- `test/description` - Test additions

### Commit Messages

Follow conventional commit format:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting)
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Maintenance tasks

**Examples:**
```
feat(atomic): add hash-based atomic implementation

Add new hash-based atomic implementation for better
scalability in high-contention scenarios.

Closes #123
```

```
fix(memory): fix memory leak in MemoryPool

The deallocate function was not properly freeing memory
in certain edge cases.

Fixes #456
```

### Workflow Steps

1. **Update Your Fork**
   ```bash
   git fetch upstream
   git checkout main
   git merge upstream/main
   ```

2. **Create Feature Branch**
   ```bash
   git checkout -b feature/my-feature
   ```

3. **Make Changes**
   - Write code
   - Add tests
   - Update documentation

4. **Commit Changes**
   ```bash
   git add .
   git commit -m "feat: add new feature"
   ```

5. **Push to Your Fork**
   ```bash
   git push origin feature/my-feature
   ```

6. **Create Pull Request**
   - Go to GitHub
   - Click "New Pull Request"
   - Fill out the template
   - Submit for review

## Coding Standards

### C++ Style

Follow the project's C++ style guidelines (see [Developer Guide](DEVELOPER_GUIDE.md)):

- Use 4 spaces for indentation
- Maximum line length: 100 characters
- Use `snake_case` for functions and variables
- Use `PascalCase` for classes
- Use `UPPER_SNAKE_CASE` for constants

### Code Formatting

Before committing, format your code:

```bash
# If using clang-format
clang-format -i src/**/*.cpp include/**/*.h

# Or use the project's formatting script
./scripts/format.sh
```

### Code Quality

- Write clear, readable code
- Add comments for complex logic
- Follow SOLID principles
- Avoid code duplication
- Handle errors properly

## Testing

### Writing Tests

1. **Unit Tests**
   - Test individual functions/classes
   - Use descriptive test names
   - Test normal and edge cases

2. **Integration Tests**
   - Test component interactions
   - Test end-to-end scenarios

3. **Performance Tests**
   - Benchmark critical paths
   - Compare before/after changes

### Running Tests

```bash
# Run all tests
cd tests
./run_all_tests.sh

# Run specific test suite
cd tests/basic
./run_tests.sh

# Run with verbose output
./run_tests.sh -v
```

### Test Requirements

- All new features must include tests
- Tests must pass before submitting PR
- Maintain or improve test coverage
- Tests should be fast and reliable

## Documentation

### Code Documentation

- Document all public APIs
- Use Doxygen-style comments
- Explain "why" not "what"
- Include usage examples

Example:
```cpp
/**
 * Allocates memory from the pool.
 * 
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory, or nullptr if pool is full
 * 
 * @note This function is thread-safe.
 * @warning Do not call from interrupt context.
 * 
 * @example
 * ```cpp
 * MemoryPool pool(1024);
 * void* ptr = pool.allocate(64);
 * if (ptr) {
 *     // Use memory
 *     pool.deallocate(ptr);
 * }
 * ```
 */
void* allocate(size_t size);
```

### User Documentation

- Update README if needed
- Add examples for new features
- Update user guide
- Add migration guides for breaking changes

## Submitting Changes

### Pull Request Checklist

Before submitting a PR, ensure:

- [ ] Code follows style guidelines
- [ ] All tests pass
- [ ] New tests are added for new features
- [ ] Documentation is updated
- [ ] Commit messages follow conventions
- [ ] Branch is up to date with main
- [ ] No merge conflicts
- [ ] Code compiles without warnings

### Pull Request Template

When creating a PR, include:

1. **Description**
   - What changes were made
   - Why the changes were needed
   - How to test the changes

2. **Related Issues**
   - Link to related issues
   - Use "Closes #123" if fixing an issue

3. **Testing**
   - How the changes were tested
   - Test results
   - Performance impact (if any)

4. **Screenshots** (if applicable)
   - UI changes
   - Performance graphs
   - Test output

### Example PR Description

```markdown
## Description

This PR adds a new hash-based atomic implementation for better
scalability in high-contention scenarios.

## Changes

- Added `HashBasedAtomic` class in `atomic/src/`
- Implemented hash table-based atomic operations
- Added unit tests for new implementation
- Updated documentation

## Testing

- All unit tests pass
- Performance tests show 20% improvement in high-contention scenarios
- Tested on x86_64 and ARM64 platforms

## Related Issues

Closes #123
```

## Review Process

### What to Expect

1. **Automated Checks**
   - CI/CD will run tests
   - Code formatting will be checked
   - Static analysis will run

2. **Code Review**
   - Maintainers will review your code
   - Feedback will be provided
   - Changes may be requested

3. **Approval**
   - Once approved, PR will be merged
   - You'll be notified of the merge

### Responding to Feedback

- Be open to feedback
- Address all comments
- Ask questions if unclear
- Update PR based on feedback
- Be patient - reviews take time

### Common Review Comments

**"Please add tests"**
- Add unit tests for new code
- Add integration tests if needed

**"Please update documentation"**
- Update relevant docs
- Add examples if needed

**"Please refactor"**
- Improve code structure
- Follow project patterns

**"Please fix style"**
- Run formatter
- Follow style guide

## Types of Contributions

### Reporting Bugs

1. **Check Existing Issues**
   - Search for similar issues
   - Check if already fixed

2. **Create Issue**
   - Use bug report template
   - Provide reproduction steps
   - Include system information
   - Add logs/output

### Suggesting Features

1. **Check Existing Issues**
   - Search for similar suggestions
   - Check roadmap

2. **Create Issue**
   - Use feature request template
   - Describe use case
   - Explain benefits
   - Provide examples

### Code Contributions

- Bug fixes
- New features
- Performance improvements
- Documentation updates
- Test additions

### Documentation Contributions

- Fix typos
- Improve clarity
- Add examples
- Translate documentation
- Create tutorials

## Getting Help

### Resources

- [Developer Guide](DEVELOPER_GUIDE.md)
- [User Guide](USER_GUIDE.md)
- [API Reference](API_REFERENCE.md)
- GitHub Discussions
- GitHub Issues

### Asking Questions

- Search existing issues/discussions
- Create new issue with question label
- Be specific about your question
- Provide context and code examples

## Recognition

Contributors will be:
- Listed in CONTRIBUTORS.md
- Credited in release notes
- Acknowledged in documentation

Thank you for contributing to SHM-PCC-SDK! 🎉

