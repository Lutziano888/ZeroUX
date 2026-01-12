#ifndef PYTHON_INTERPRETER_H
#define PYTHON_INTERPRETER_H

// Führt ein Skript aus und schreibt das Ergebnis in output_buffer
void python_run_script(const char* script, char* output_buffer, int max_out_len);

#endif