#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAXLEN 256

// Token types
typedef enum {
    UNKNOWN, END, ENDFILE, 
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN, 
    LPAREN, RPAREN,INCDEC,AND,OR,XOR,
    ADDSUB_ASSIGN
} TokenSet;

// Test if a token matches the current token 
int match(TokenSet token);

// Get the next token
void advance(void);

// Get the lexeme of the current token
char *getLexeme(void);
#define TBLSIZE 64

// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 0

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    if (PRINTERR) \
        fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
    err(errorNum); \
}

// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR
} ErrorType;

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left; 
    struct _Node *right;
} BTNode;

// The symbol table
Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
void initTable(void);

// Get the value of a variable
int getval(char *str);

// Set the value of a variable
int setval(char *str, int val);

// Make a new node according to token type and lexeme
BTNode *makeNode(TokenSet tok, const char *lexe);

// Free the syntax tree
void freeTree(BTNode *root);

BTNode *factor(void);
BTNode *assign_expr(void);
BTNode *or_expr(void);
BTNode *or_expr_tail(BTNode *left);
BTNode *xor_expr(void);
BTNode *xor_expr_tail(BTNode *left);
BTNode *and_expr(void);
BTNode *and_expr_tail(BTNode *left);
BTNode *addsub_expr(void);
BTNode *addsub_expr_tail(BTNode *left);
BTNode *muldiv_expr(void);
BTNode *muldiv_expr_tail(BTNode *left);
BTNode *unary_expr(void);
void statement(void);

// Print error message and exit the program
void err(ErrorType errorNum);

// Evaluate the syntax tree
int evaluateTree(BTNode *root);

// Print the syntax tree in prefix
void printPrefix(BTNode *root);

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];

TokenSet getToken(void)
{
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t');

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if (c == '+' || c == '-') {
        lexeme[0] = c;
        c = fgetc(stdin);
        if(c==lexeme[0]){
            lexeme[1] = c;
            lexeme[2] = '\0';
            return INCDEC;
        }
        else if(c=='='){
            lexeme[1] = '=';
            lexeme[2] = '\0';
            return ADDSUB_ASSIGN;
        }
        ungetc(c,stdin);
        lexeme[1] = '\0';
        return ADDSUB;
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (isalpha(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while ((isdigit(c) || isalpha(c) || c=='_')&& i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    } else if (c == EOF) {
        return ENDFILE;
    } 
    else if (c == '&') {
        strcpy(lexeme, "&");
        return AND;
    } 
    else if (c == '|') {
        strcpy(lexeme, "|");
        return OR;
    } 
    else if (c == '^') {
        strcpy(lexeme, "^");
        return XOR;
    } 
    else {
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void) {
    return lexeme;
}

int sbcount = 0;
int varpos = 0;
int regcnt = 0;
Symbol table[TBLSIZE];

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    sbcount = 3;
}

int getval(char *str) {
    int i = 0;

    for (i = 0; i < sbcount; i++)
        if (strcmp(str, table[i].name) == 0){
            varpos = i*4;
            return table[i].val;
        }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    
    error(UNDEFINED);
    return 0;
}

int setval(char *str, int val) {
    int i = 0;

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            table[i].val = val;
            return val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    
    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    sbcount++;
    return val;
}

BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// factor := INT | ADDSUB INT |
//		   	 ID  | ADDSUB ID  | 
//		   	 ID ASSIGN expr |
//		   	 LPAREN expr RPAREN |
//		   	 ADDSUB LPAREN expr RPAREN
BTNode *factor(void) {
    BTNode *retp = NULL, *left = NULL;
    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        advance();
    } else if (match(ID)) {
        retp = makeNode(ID, getLexeme());
        advance();
    } else if (match(INCDEC)) {
        retp = makeNode(INCDEC, getLexeme());
        retp->right = makeNode(INT, "1");
        advance();
        if (match(ID)) {
            retp->left = makeNode(ID, getLexeme());
            advance();
        } 
    } else if (match(LPAREN)) {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            error(MISPAREN);
    } else {
        error(NOTNUMID);
    }
    return retp;
}

BTNode *assign_expr(void){
    BTNode *node = NULL;
    BTNode *left = or_expr();
    if (match(ASSIGN)) {
        if(left->data!=ID){
            err(SYNTAXERR);
        }
        node = makeNode(ASSIGN, getLexeme());
        advance();
        node->left = left;
        node->right = assign_expr();
        return node;
    } 
    else if (match(ADDSUB_ASSIGN)) {
        if(left->data!=ID){
            err(SYNTAXERR);
        }
        node = makeNode(ADDSUB_ASSIGN, getLexeme());
        advance();
        node->left = left;
        node->right = assign_expr();
        return node;
    } 
    else {
        return left;
    }
}
BTNode *or_expr(void){
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}
BTNode *or_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(OR)) {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        return or_expr_tail(node);
    } else {
        return left;
    }
}
BTNode *xor_expr(void){
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}
BTNode *xor_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(XOR)) {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        return xor_expr_tail(node);
    } else {
        return left;
    }
}
BTNode *and_expr(void){
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}
BTNode *and_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(AND)) {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        return and_expr_tail(node);
    } else {
        return left;
    }
}
BTNode *addsub_expr(void){
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}
BTNode *addsub_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        return addsub_expr_tail(node);
    } else {
        return left;
    }
}
BTNode *muldiv_expr(void){
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}
BTNode *muldiv_expr_tail(BTNode *left){
    BTNode *node = NULL;
    if (match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        return muldiv_expr_tail(node);
    } else {
        return left;
    }
}
BTNode *unary_expr(void){
    BTNode *node = NULL;
    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = makeNode(INT, "0");
        node->right = unary_expr();
        return node;
    } else {
        return factor();
    }
}

// statement := ENDFILE | END | expr END
void statement(void) {
    BTNode *retp = NULL;
    regcnt = 0;
    if (match(ENDFILE)) {
        printf("MOV r0 [0]\n");
        printf("MOV r1 [4]\n");
        printf("MOV r2 [8]\n");
        printf("EXIT 0\n");
        exit(0);
    } 
    else {
        retp = assign_expr();
        if (match(END)) {
            if(PRINTERR==0){
                evaluateTree(retp);
                freeTree(retp);
            }
            else{
            printf("%d\n", evaluateTree(retp));
            printf("Prefix traversal: ");
            printPrefix(retp);
            printf("\n");
            freeTree(retp);
            printf(">> ");
            }
            advance();
        } else {
            error(SYNTAXERR);
        }
    }
}

void err(ErrorType errorNum) {
    if (PRINTERR) {
        fprintf(stderr, "error: ");
        switch (errorNum) {
            case MISPAREN:
                fprintf(stderr, "mismatched parenthesis\n");
                break;
            case NOTNUMID:
                fprintf(stderr, "number or identifier expected\n");
                break;
            case NOTFOUND:
                fprintf(stderr, "variable not defined\n");
                break;
            case RUNOUT:
                fprintf(stderr, "out of memory\n");
                break;
            case NOTLVAL:
                fprintf(stderr, "lvalue required as an operand\n");
                break;
            case DIVZERO:
                fprintf(stderr, "divide by constant zero\n");
                break;
            case SYNTAXERR:
                fprintf(stderr, "syntax error\n");
                break;
            default:
                fprintf(stderr, "undefined error\n");
                break;
        }
    }
    printf("EXIT 1\n");
    exit(0);
}
int idcnt = 0;
int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;

    if (root != NULL) {
        switch (root->data) {
            case ID:
                idcnt++;
                retval = getval(root->lexeme);
                if(regcnt>7){
                    printf("MOV [%d] r%d\n", sbcount<<2, regcnt%8);
                    sbcount++;
                }
                printf("MOV r%d [%d]\n", (regcnt)%8, varpos);
                regcnt++;
                break;
            case INT:
                retval = atoi(root->lexeme);
                if(regcnt>7){
                    printf("MOV [%d] r%d\n", sbcount<<2, regcnt%8);
                    sbcount++;
                }
                printf("MOV r%d %d\n", (regcnt)%8, retval);
                regcnt++;
                break;
            case ASSIGN:
                rv = evaluateTree(root->right);
                retval = setval(root->left->lexeme, rv);
                getval(root->left->lexeme);
                printf("MOV [%d] r%d\n", varpos,(regcnt-1)%8);
                break;
            case MULDIV:
                lv = evaluateTree(root->left);
                idcnt = 0;
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                    printf("MUL r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                } else if (strcmp(root->lexeme, "/") == 0) {
                    if (rv == 0){
                        if(idcnt==0) error(DIVZERO);
                    }
                    else{
                        retval = lv / rv;
                    }
                    printf("DIV r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                }
                if(regcnt>8){
                    sbcount--;
                    printf("MOV r%d [%d]\n", (regcnt-1)%8, sbcount<<2);
                }
                regcnt--;
                break;
            case ADDSUB_ASSIGN:
                lv = evaluateTree(root->left);
                int varpostmp = varpos;
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "+=") == 0) {
                    retval = setval(root->left->lexeme, lv+rv);
                    printf("ADD r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                    printf("MOV [%d] r%d\n", varpostmp, (regcnt-2)%8);
                } 
                else if (strcmp(root->lexeme, "-=") == 0) {
                    retval = setval(root->left->lexeme, lv-rv);
                    printf("SUB r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                    printf("MOV [%d] r%d\n", varpostmp, (regcnt-2)%8);
                } 
                if(regcnt>8){
                    sbcount--;
                    printf("MOV r%d [%d]\n", (regcnt-1)%8, sbcount<<2);
                }
                regcnt--;
                break;
            case ADDSUB:
            case AND:
            case OR:
            case XOR:
            case INCDEC:
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                    printf("ADD r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                } else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                    printf("SUB r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                } 
                else if (strcmp(root->lexeme, "&") == 0) {
                    retval = lv & rv;
                    printf("AND r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                } 
                else if (strcmp(root->lexeme, "|") == 0) {
                    retval = lv | rv;
                    printf("OR r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                } 
                else if (strcmp(root->lexeme, "^") == 0) {
                    retval = lv ^ rv;
                    printf("XOR r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                } 
                else if (strcmp(root->lexeme, "++") == 0) {
                    retval = setval(root->left->lexeme, lv+1);
                    printf("ADD r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                    printf("MOV [%d] r%d\n", varpos, (regcnt-2)%8);
                } 
                else if (strcmp(root->lexeme, "--") == 0) {
                    retval = setval(root->left->lexeme, lv-1);
                    printf("SUB r%d r%d\n", (regcnt-2)%8, (regcnt-1)%8);
                    printf("MOV [%d] r%d\n", varpos, (regcnt-2)%8);
                } 
                if(regcnt>8){
                    sbcount--;
                    printf("MOV r%d [%d]\n", (regcnt-1)%8, sbcount<<2);
                }
                regcnt--;
                break;
            default:
                retval = 0;
        }
    }
    return retval;
}

void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}
int main() {
    initTable();
    while (1) {
        statement();
    }
    return 0;
}