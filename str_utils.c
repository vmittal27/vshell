#include "str_utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdarg.h>

int is_nonspace(char ch)
{
    return !( 9 <= ch && ch <= 13) && ch != 32;
}

char *trim_str(char *s)
{
    int slen = strlen(s); 
    char buffer[slen + 1];
    memset(buffer, 0, slen + 1); 

    int left = 0;
    int right = slen - 1; 

    // trim leading whitespace
    while (left < slen && !is_nonspace(s[left]))
        ++left;
    // trim trailing whitespace
    while (right > 0 && !is_nonspace(s[right]))
        --right;

    for (int i = left; i <= right; i++)
        buffer[i - left] = s[i];

    return strdup(buffer); 
}

char *string_splice(char *s, int start, int end) {
    size_t slen = strlen(s); 
    if (start < 0 || end > slen|| start > end)
        return NULL;
    
    int new_len = end - start; 
    char *spliced_s = calloc(new_len + 1, sizeof(char)); 
    for (int i = 0; i < new_len; i++) {
        spliced_s[i] = s[start + i]; 
    }
    return spliced_s; 
}

/*Given a string `s`, split it into a null-terminated array along the character `delim`.
This implementation uses strstr() internally to split.
The length of the split string is encoded into `split_len`.*/
char **__split_str_by_str(char *s, char *delim, int *split_len) {
    char *start = s; 
    char *cur_pos; 
    size_t delim_len = strlen(delim);

    // We first calculate the necessary length of the split array
    *split_len = 0; 

    while ((cur_pos = strstr(start, delim)) != NULL) {
        (*split_len)++;
        start = cur_pos + delim_len; 
    }

    if (strlen(start)) {
        (*split_len)++;
    }

    char **split_s = malloc(sizeof(char*) * (*split_len + 1));

    int i = 0;
    start = s; 

    while ((cur_pos = strstr(start, delim)) != NULL) {
        int end = cur_pos - start; 
        split_s[i++] = string_splice(start, 0, end); 
        start = cur_pos + delim_len; 
    }

    // This does not include the last substring, so we account for that
    split_s[i] = string_splice(start, 0, strlen(start)); 

    split_s[*split_len] = NULL; 
    return split_s; 

}

/*Given a string `s`, split it into a null-terminated array along the string `delim`.
Because this implementation uses `strtok()` internally, 
it will split along any character in `delim`.
The length of the split string is encoded into `split_len`.*/
char **__split_str_by_char(char *s, char *delim, int *split_len)
{
    char *s_copy;
    *split_len = 0; 
    char *token; 

    // We first determine the length of the split.
    // Since strtok modifies the original string,
    // we create a copy first.
    s_copy = strdup(s); 
    token = strtok(s_copy, delim);
    while (token != NULL) {
        (*split_len)++; 
        token = strtok(NULL, delim); 
    }
    free(s_copy); 

    // then we allocate the array of str pointers
    char **split_s = malloc(sizeof(char*) * (*split_len + 1));

    // now we create another copy to do the actual allocation
    s_copy = strdup(s);

    int i = 0;
    token = strtok(s_copy, delim);
    while (token != NULL) {
        split_s[i++] = strdup(token); 
        token = strtok(NULL, delim);
    }
    free(s_copy); 

    split_s[*split_len] = NULL; // null terminates array
    return split_s; 
}

char **split_str(char *s, char *delim, int *split_len) {
    if (strlen(delim) == 1) {
        return __split_str_by_char(s, delim, split_len); 
    }
    else {
        return __split_str_by_str(s, delim, split_len);
    }
}

void free_str_array(char **s_arr) {
    int i = 0; 
    while (s_arr[i] != NULL) {
        free(s_arr[i]);
        ++i; 
    }
}