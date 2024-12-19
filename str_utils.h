/*
Checks if a character is a non-space character.
*/
int is_nonspace(char ch);

/*Returns a dynamically allocated copy of `s` that is free
of all leading and trailing whitespaces.
*/
char *trim_str(char *s);

/*Given a null-terminated string `s`, index `start`, and index `end`, 
return a dynamically allocated, null-terminated substring of `s` 
from `start` to `end` (non-inclusive). 
If `start < 0`, `end > strlen(s)`, or `start > end`, returns `NULL`.*/
char *string_splice(char *s, int start, int end);

/*Splits a string `s` using `delim` as delimters. 
This function returns a null-terminated array and stores
the length (not including the null-terminator) in `split_len`.*/
char **split_str(char *s, char *delim, int *split_len);

/*Frees a null-terminated array of strings.*/
void free_str_array(char **s_arr); 
