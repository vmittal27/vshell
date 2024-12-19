# V Shell (vsh)

This is a custom UNIX shell I wrote that offers all the basic functionality
other shell environments like Z shell and Bash offer.
You can run all commands (including running multiple commands per line with a
`;` separator) and redirect output in one of 3 ways: 
1. Standard redirection (>): redirect any output to a new file.
2. Append redirection (>>): redirect any output to the end of an existing file.
3. Prepend redirection (>+): redirect any output to the beginning of a file.

## Repository Structure

`str_utils.c`: contains a number of string utilities to aid in parsing input.

`vshell.c`: orchestrates the shell by parsing and validating input before
forking the parent process and executing the program specified by the user.

## Usage
To build, run make with the current working directory set to the root of the
repository. This will generate the `vsh` executable. From there, run `./vsh`.
The shell will initialize in the machine's home directory.
To exit out of the shell, run the command `exit` or type `ctrl + C`. 

## Examples
```zsh
(vsh) vireshmittal@Vireshs-MacBook-Pro.local:/Users/vireshmittal > pwd
/Users/vireshmittal
(vsh) vireshmittal@Vireshs-MacBook-Pro.local:/Users/vireshmittal > echo test; echo another test
test
another test
(vsh) vireshmittal@Vireshs-MacBook-Pro.local:/Users/vireshmittal > exit
```
