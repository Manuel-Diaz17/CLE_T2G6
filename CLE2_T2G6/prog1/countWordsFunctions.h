#ifndef COUNTWORDSFUNCTIONS_H
#define COUNTWORDSFUNCTIONS_H

// Function to check if a char is vowel
extern int is_vowel(unsigned char *c);

// Function to check if a char is consonant
extern int is_consonant(unsigned char *c);

// Function to check if a char is decimal digit
extern int is_decimal_digit(unsigned char *c);

// Function to check if a char is _
extern int is_underscore(unsigned char *c);

// Function to check if a char is a whitespace symbol
extern int is_whitespace(unsigned char *c);

// Function to check if a char is a separation symbol
extern int is_separation(unsigned char *c);

// Function to check if a char is a punctuation symbol
extern int is_punctuation(unsigned char *c);

// Function to check if a char is apostrophe
extern int is_apostrophe(unsigned char *c);

// Function to convert multibyte chars to singlebyte chars
extern char convert_special_chars(unsigned char c);

#endif /* COUNTWORDSFUNCTIONS_H */