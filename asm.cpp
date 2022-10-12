#include "proc.h"


int main (int argc, char *argv[])
{
    const char * input_file_name = nullptr;
    const char *output_file_name = nullptr;

    if (argc >= 2)  input_file_name = argv [1];
    else            input_file_name = "in.txt";

    if (argc >= 3) output_file_name = argv [2];
    else           output_file_name =      "a";


    cmd_t *commands = nullptr;
    struct Text txt = {};

    int err = OK;

    err = ReadText (input_file_name, &txt);
    if (err) return err;

    err = SetCmds (&txt, &commands);
    if (err) return err;

    FreeText (&txt);

    err = WriteCmds (output_file_name, commands);
    if (err) return err;

    free (commands);

    return OK;
}

//---------------------------------------------------------------------------------------------------------------------

int ReadText (const char *input_file_name, struct Text *txt)
{    
    if (input_file_name == nullptr) return NULLPTR_ARG;
    if             (txt == nullptr) return NULLPTR_ARG;

    FILE *inp_file = fopen (input_file_name, "r");
    if (inp_file == nullptr) return FOPEN_ERROR;

    size_t filesize = GetSize (inp_file);

    txt -> buffer = (char *) calloc (filesize + 1, sizeof((txt -> buffer)[0]));
    if ((txt -> buffer) == nullptr) return ALLOC_ERROR;

    txt -> buflen = fread (txt -> buffer, sizeof((txt -> buffer)[0]), filesize, inp_file);
    *(txt -> buffer + txt -> buflen) = '\0';
    
    fclose (inp_file);

    txt -> len = CharReplace (txt -> buffer, '\n', '\0') + 1;

    SetLines (txt);

    return OK;
}


size_t GetSize (FILE *inp_file)
{
    if (inp_file == nullptr) return 0;
    struct stat stat_buf = {};

    fstat (fileno (inp_file), &stat_buf);
    return stat_buf.st_size;
}


size_t CharReplace (char *str, char ch1, char ch2)
{
    if (str == nullptr) return 0;

    size_t count = 0;
    str = strchr (str, ch1);

    while (str != nullptr)
    {
        count++;
        *str = ch2;
        str = strchr (str + 1, ch1);
    }

    return count;
}


int SetLines (struct Text *txt)
{
    if (txt           == nullptr) return NULLPTR_ARG;
    if (txt -> buffer == nullptr) return NULLPTR_ARG;

    txt -> lines = (char **) calloc (txt -> len, sizeof ((txt -> lines)[0]));
    if ((txt -> lines) == nullptr) return ALLOC_ERROR;

    char *str_ptr = txt -> buffer;

    for (size_t index = 0; index < txt -> len; index++)
    {
        txt -> lines [index] = str_ptr;
        str_ptr += strlen (str_ptr) + 1;
    }

    return OK;
}

void FreeText (struct Text *txt)
{
    free (txt -> buffer);
    free (txt ->  lines);
}

//---------------------------------------------------------------------------------------------------------------------

int SetCmds (struct Text *txt, cmd_t **cmds_p)
{
    if (txt           == nullptr) return NULLPTR_ARG;
    if (txt -> buffer == nullptr) return NULLPTR_ARG;
    if (txt ->  lines == nullptr) return NULLPTR_ARG;
    if (cmds_p        == nullptr) return NULLPTR_ARG;

    size_t size = (MAX_NUM_OF_ARGS * ARG_SIZE + CMD_SIZE) * txt -> len + INFO_SIZE; 

    *cmds_p = (cmd_t *) (calloc (1, size));

    cmd_t *cmds = *cmds_p;

    if (cmds == nullptr) return ALLOC_ERROR;

    cmds [0] = SIGNATURE;
    cmds [1] = VERSION;

    int *cmd_ptr = cmds + 3;

    for (size_t line = 0; line < txt -> len; line++)
    {
        int symbs_read = 0;
        char cmd [BUFLEN] = "";

        sscanf (txt -> lines [line], "%s%n", cmd, &symbs_read);

        if (stricmp (cmd, "") == 0) continue;

        if      (stricmp (cmd, "PUSH") == 0)
        {   
            *(cmd_ptr++) |= CMD_PUSH;
            if (GetArgs (txt -> lines [line] + symbs_read, &cmd_ptr, line)) return COMP_ERROR; 
        }
        else if (stricmp (cmd,  "POP") == 0)
        {
            *(cmd_ptr++) |= CMD_POP;
            if (GetArgs (txt -> lines [line] + symbs_read, &cmd_ptr, line)) return COMP_ERROR;
        }
        else if (stricmp (cmd,   "IN") == 0) *(cmd_ptr++) |= CMD_IN;
        else if (stricmp (cmd,  "OUT") == 0) *(cmd_ptr++) |= CMD_OUT;
        else if (stricmp (cmd,  "ADD") == 0) *(cmd_ptr++) |= CMD_ADD;
        else if (stricmp (cmd,  "SUB") == 0) *(cmd_ptr++) |= CMD_SUB;
        else if (stricmp (cmd,  "MUL") == 0) *(cmd_ptr++) |= CMD_MUL;
        else if (stricmp (cmd,  "DIV") == 0) *(cmd_ptr++) |= CMD_DIV;
        else if (stricmp (cmd,  "HLT") == 0) *(cmd_ptr++) |= CMD_HLT;
        else
        {
            fprintf (ERROR_STREAM, "Compilation error:\nunknown command at line (%Iu):\n(%s)\n", line + 1, cmd);
            return COMP_ERROR;
        }
    }

    cmds[2] = cmd_ptr - cmds;

    return OK;
}

int GetArgs (char *args, cmd_t **cmd_ptr_p, size_t line)
{
    cmd_t *cmd_ptr = *cmd_ptr_p;

    args = DeleteSpaces (args);

    if (*args == '\0')
    {
        fprintf (ERROR_STREAM, "Compilation error:\nmissing argument at line (%Iu)\n", line + 1);
        return COMP_ERROR;
    }

    size_t len = strlen (args);

    if (strchr (args, '['))
    {
        if (strchr (args, ']') != args + len - 1 || strchr (args + 1, '['))
        {
            fprintf (ERROR_STREAM, "Compilation error:\nincorrect argument format at line (%Iu)\n", line + 1);
            return COMP_ERROR;
        }

        args [len - 1] = '\0';
        args++;
        len -= 2;

        *(cmd_ptr - 1) |= ARG_MEM;
    }
    
    char *arg1 = args;
    char *arg2 = strchr (args, '+');

    int got_im  = 0;
    int got_reg = 0;

    if (arg2)
    {
        *(arg2++) = '\0';
        arg2 = DeleteSpaces (arg2);

        if (arg2 [0] == 'r' && strlen (arg2) == 3 && arg2 [2] == 'x')
        {
            *(cmd_ptr++) = arg2 [1] - 'a' + 1;
            got_reg = 1;
        }
        else if (isdigit (arg2 [0]))
        {
            sscanf (arg2, "%d", cmd_ptr + 1);
            got_im = 1;
        }
        else
        {
            fprintf (ERROR_STREAM, "Compilation error:\nincorrect argument format at line (%Iu)\n", line + 1);
            return COMP_ERROR;
        }
    }

    arg1 = DeleteSpaces (arg1);

    if (!got_reg && arg1 [0] == 'r' && strlen (arg1) == 3 && arg1 [2] == 'x')
    {
        *(cmd_ptr++) = arg1 [1] - 'a' + 1;
        got_reg = 1;
        if (got_im) cmd_ptr++;
    }
    else if (!got_im && isdigit (arg1 [0]))
    {
        sscanf (arg1, "%d", cmd_ptr++);
        got_im = 1;
    }
    else
    {
        fprintf (ERROR_STREAM, "Compilation error:\nincorrect argument format at line (%Iu)\n", line + 1);
        return COMP_ERROR;
    }

    if (arg2) *(cmd_ptr - 3) |= ARG_IM | ARG_REG;
    else      *(cmd_ptr - 2) |= got_im ? ARG_IM : ARG_REG;

    *cmd_ptr_p = cmd_ptr;

    return OK;
}

char *DeleteSpaces (char *str)
{
    if (str == nullptr) return nullptr;

    while (isspace (*str)) str++;

    size_t len = strlen (str);

    while (isspace (str [len - 1]) && len > 0) len--;
    str [len] = '\0';

    return str;
}

int WriteCmds (const char *output_file_name, cmd_t *cmds)
{
    if (output_file_name == nullptr) return NULLPTR_ARG;
    if             (cmds == nullptr) return NULLPTR_ARG;

    FILE *out_file = fopen (output_file_name, "wb");
    if (out_file == nullptr) return FOPEN_ERROR;

    if (fwrite ((void *) cmds, CMD_SIZE, cmds [2], out_file) != cmds [2]) return FWRITE_ERROR;

    fclose (out_file);

    return OK;
}