#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>

int is_vowel(unsigned char *c) { 
    if (*c == 'a' || *c == 'e' || *c == 'i' || *c == 'o' || *c == 'u' ||
        *c == 'A' || *c == 'E' || *c == 'I' || *c == 'O' || *c == 'U') {
        return 1;
    } else {
        return 0;
    }
}

int is_consonant(unsigned char *c) {
    if (!is_vowel(c) && isalpha(*c)) {
        return 1;
    } else {
        return 0;
    }
}

int is_decimal_digit(unsigned char *c) {
    if (isdigit(*c)) {
        return 1;
    } else {
        return 0;
    }
}

int is_underscore(unsigned char *c) {
    if (*c == '_') {
        return 1;
    } else {
        return 0;
    }
}

int is_whitespace(unsigned char *c) {
    if ( *c == 0x20 || *c == 0x9 || *c == 0xA || *c == 0xD){
        return 1;
    } else {
        return 0;
    }
}

int is_separation(unsigned char *c) {
    if (*c == '-' || *c == '"' || *c == '[' || *c == ']' || *c == '(' || *c == ')') {
        return 1;
    }
    if (*c == 0xE2 && *(c+1) == 0x80 && *(c+2) == 0x9C) {   // open double quotation mark 
        return 1;
    }
    if (*c == 0xE2 && *(c+1) == 0x80 && *(c+2) == 0x9D) {   // close double quotation mark
        return 1;
    }
    return 0;
}

int is_punctuation(unsigned char *c) {
    if (*c == '.' || *c == ',' || *c == ':' || *c == ';' || *c == '?' || *c =='!') {
        return 1;
    }
    if (*c == 0xE2 && *(c+1) == 0x80 && *(c+2) == 0x93) {   // dash
        return 1;
    }
    if (*c == 0xE2 && *(c+1) == 0x80 && *(c+2) == 0xA6) {   // ellipsis
        return 1;
    }
    return 0;
}

int is_apostrophe(unsigned char *c) {
    if (*c == 0x27) {
        return 1;
    }
    if (*c == 0xE2 && *(c+1) == 0x80 && *(c+2) == 0x98) {   // open single quotation mark
        return 1;
    }
    if (*c == 0xE2 && *(c+1) == 0x80 && *(c+2) == 0x99) {   // close single quotation mark
        return 1;
    }
    return 0;
}

char convert_special_chars(unsigned char c) {
    switch (c) {
        // á à â ã - a
        case 0xA1: c=0x61; break;
        case 0xA0: c=0x61; break;
        case 0xA2: c=0x61; break;
        case 0xA3: c=0x61; break;
        // Á À Â Ã - A
        case 0x81: c=0x41; break;
        case 0x80: c=0x41; break;
        case 0x82: c=0x41; break;
        case 0x83: c=0x41; break;
        // é è ê - e
        case 0xA9: c=0x65;break;
        case 0xA8: c=0x65;break;
        case 0xAA: c=0x65;break;
        // É È Ê - E
        case 0x89: c=0x45;break;
        case 0x88: c=0x45;break;
        case 0x8A: c=0x45;break;
        // í ì - i
        case 0xAD: c=0x69;break;
        case 0xAC: c=0x69;break;
        // Í Ì - I
        case 0x8D: c=0x49;break;
        case 0x8C: c=0x49;break;
        // ó ò ô õ - o
        case 0xB3: c=0x6F; break;
        case 0xB2: c=0x6F; break;
        case 0xB4: c=0x6F; break;
        case 0xB5: c=0x6F; break;
        // Ó Ò Ô Õ - O
        case 0x93: c=0x4F; break;
        case 0x92: c=0x4F; break;
        case 0x94: c=0x4F; break;
        case 0x95: c=0x4F; break;
        // ú ù - u
        case 0xBA: c=0x75;break;
        case 0xB9: c=0x75;break;
        // Ú Ù - U
        case 0x9A: c=0x55;break;
        case 0x99: c=0x55;break;
        // Ç ç - C
        case 0xA7: c=0x63;break;
        case 0x87: c=0x43;break;

        default:    break;
    }
    return c;
}