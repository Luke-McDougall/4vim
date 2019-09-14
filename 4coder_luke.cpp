//=============================================================================
// >>> my vim-based 4coder custom <<<
// author: chr <chr@chronal.net>
//
// Sample usage of vim functions, from my own 4coder custom. 
// Feel free to copy and tweak as you like!
//=============================================================================
#ifdef DEBUG
#include <stdio.h>
#endif

#include <stdlib.h>
#include "4coder_vim.cpp"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define internal static

// These colors are tuned to work with the Dragonfire color scheme.
// TODO(chr): How to best make this configurable? Can we query for arbitrary
// variables in the theme?
constexpr int_color color_margin_normal = 0xFF341313;
constexpr int_color color_margin_insert = 0xFF5a3619;
constexpr int_color color_margin_replace = 0xFF5a192e;
constexpr int_color color_margin_visual = 0xFF722b04;

START_HOOK_SIG(luke_init) {
    default_4coder_initialize(app);
    default_4coder_side_by_side_panels(app, files, file_count);
    change_theme(app, literal("Handmade Hero"));
    // NOTE(chr): Be sure to call the vim custom's hook!
    return vim_hook_init_func(app, files, file_count, flags, flag_count);
}

// NOTE(chr): Define the four functions that the vim plugin wants in order
// to determine what to do when modes change.
// TODO(chr): 
void on_enter_insert_mode(struct Application_Links *app) {
    Theme_Color colors[] = {
        { Stag_Bar_Active, color_margin_insert },
        { Stag_Margin_Active, color_margin_insert },
    };
    set_theme_colors(app, colors, ArrayCount(colors));
}

void on_enter_replace_mode(struct Application_Links *app) {
    Theme_Color colors[] = {
        { Stag_Bar_Active, color_margin_replace },
        { Stag_Margin_Active, color_margin_replace },
    };
    set_theme_colors(app, colors, ArrayCount(colors));
}

void on_enter_normal_mode(struct Application_Links *app) {
    Theme_Color colors[] = {
        { Stag_Bar_Active, color_margin_normal },
        { Stag_Margin_Active, color_margin_normal },
    };
    set_theme_colors(app, colors, ArrayCount(colors));
}

void on_enter_visual_mode(struct Application_Links *app) {
    Theme_Color colors[] = {
        { Stag_Bar_Active, color_margin_visual },
        { Stag_Margin_Active, color_margin_visual },
    };
    set_theme_colors(app, colors, ArrayCount(colors));
}

typedef enum
{
    negative,
    minus,
    plus,
    multiply,
    divide,
    open_bracket,
    close_bracket,
    constant
} Type;

typedef struct
{
    Type type;
    double value;
} Entry;

typedef struct
{
    Entry *data;
    u32 capacity;
    u32 end;
} Stack;

internal Stack*
new_stack(u32 capacity)
{
    Stack *s = (Stack*)malloc(sizeof(Stack));
    s->capacity = capacity;
    s->data = (Entry*)malloc(sizeof(Entry) * capacity);
    return s;
}

internal int
push(Stack *s, Entry e)
{
    if(s && (s->end < s->capacity))
    {
        s->data[s->end] = e;
        s->end++;
        return 1;
    }
    return 0;
}

// Pass NULL for the Entry pointer if you don't care about the value and just want to remove it from the stack.
internal int
pop(Stack *s, Entry *e)
{
    if(s)
    {
        if(s->end > 0)
        {
            if(e == NULL)
            {
                s->end--;
            }
            else
            {
                *e = s->data[s->end - 1];
                s->end--;
            }
            return 1;
        }
    }
    return 0;
}

internal int
top(Stack *s, Entry *e)
{
    if(s && e)
    {
        if(s->end > 0)
        {
            *e = s->data[s->end - 1];
            return 1;
        }
    }
    return 0;
}

internal int
get(Stack *s, Entry *e, u32 index)
{
    if((s && e) && (s->end > index))
    {
        *e = s->data[index];
        return 1;
    }
    return 0;
}

internal void
free_stack(Stack *s)
{
    if(s)
    {
        if(s->data)
        {
            free(s->data);
        }
        free(s);
    }
}

internal bool
precedence(Type t1, Type t2)
{
    if(t1 == negative) return true;
    if((t1 == divide || t1 == multiply) && t2 != negative) return true;
    if((t1 == plus || t1 == minus) && (t2 == plus && t2 == minus)) return true;
    return false;
}

internal double
operation(double r1, double r2, Type t)
{
    switch(t)
    {
        case plus:
        return r1 + r2;
        break;
        
        case minus:
        return r1 - r2;
        break;
        
        case multiply:
        return r1 * r2;
        break;
        
        case divide:
        return r1 / r2;
        break;
        
        case negative:
        return -r1;
        break;
    }
    return 0;
}

internal double
solve_equation(char *equation, size_t length)
{
    Stack *infix = new_stack((u32) length);
    u32 eqn_idx = 0;
    char num_buf[128];
    
    while(eqn_idx < length)
    {
        char c = equation[eqn_idx++];
        Entry e;
        
        if(c == '-' || c == '+' || c == '*' || c == '/' || c == '(' || c == ')')
        {
            switch(c)
            {
                case '-':
                {
                    if(top(infix, &e))
                    {
                        if(e.type != constant)
                        {
                            e.type = negative;
                        }
                        else
                        {
                            e.type = minus;
                        }
                    }
                    else
                    {
                        e.type = negative;
                    }
                } break;
                
                case '+':
                {
                    e.type = plus;
                } break;
                
                case '*':
                {
                    e.type = multiply;
                } break;
                
                case '/':
                {
                    e.type = divide;
                } break;
                
                case '(':
                {
                    e.type = open_bracket;
                } break;
                
                case ')':
                {
                    e.type = close_bracket;
                } break;
            }
            push(infix, e);
        }
        else if(c >= '0' && c <= '9')
        {
            u32 buf_idx = 0;
            while((c >= '0' && c <= '9') || c == '.')
            {
                num_buf[buf_idx++] = c;
                c = equation[eqn_idx++];
            }
            eqn_idx--;
            
            e.type = constant;
            e.value = strtod(num_buf, NULL);
            push(infix, e);
            
            for(int i = 0; i < buf_idx; i++)
            {
                num_buf[i] = 0;
            }
        }
    }
    
    // Convert to postfix
    
    Stack *postfix = new_stack(infix->end);
    Stack *op = new_stack(infix->end);
    
    for(int i = 0; i < infix->end; i++)
    {
        Entry e;
        get(infix, &e, i);
        
        if(e.type == constant)
        {
            push(postfix, e);
        }
        else if(e.type == open_bracket)
        {
            push(op, e);
        }
        else if(e.type == close_bracket)
        {
            for(;;)
            {
                Entry o;
                pop(op, &o);
                if(o.type == open_bracket) break;
                
                // TODO(Luke): Implement proper input validation so I won't need this anymore
                if(op->end <= 0) break;
                
                push(postfix, o);
            }
        }
        else
        {
            if(op->end == 0)
            {
                push(op, e);
            }
            else
            {
                Entry o;
                top(op, &o);
                
                if(o.type == open_bracket)
                {
                    push(op, e);
                }
                else
                {
                    if(precedence(o.type, e.type))
                    {
                        pop(op, &o);
                        push(postfix, o);
                        push(op, e);
                    }
                    else
                    {
                        push(op, e);
                    }
                }
            }
        }
    }
    
    while(op->end > 0)
    {
        Entry o;
        pop(op, &o);
        push(postfix, o);
    }
    
#ifdef DEBUG
    FILE *f = fopen("quick_calc.debug.log", "a");
    fprintf(f, "\nContents of postfix stack\n");
    for(int i = 0; i < postfix->end; i++)
    {
        Entry e;
        get(postfix, &e, i);
        if(e.type == constant)
        {
            fprintf(f, "constant:%lf, ", e.value);
        }
        else
        {
            switch(e.type)
            {
                case plus:
                fprintf(f, "plus, ");
                break;
                
                case minus:
                fprintf(f, "minus, ");
                break;
                
                case multiply:
                fprintf(f, "multiply, ");
                break;
                
                case divide:
                fprintf(f, "divide, ");
                break;
                
                case negative:
                fprintf(f, "negative, ");
                break;
            }
        }
    }
    fclose(f);
#endif
    
    Stack *constants = new_stack(postfix->end);
    
    for(int i = 0; i < postfix->end; i++)
    {
        Entry e;
        get(postfix, &e, i);
        
        if(e.type == constant)
        {
            push(constants, e);
        }
        else
        {
            if(e.type == negative)
            {
                Entry r;
                pop(constants, &r);
                r.value = operation(r.value, 0, e.type);
            }
            else
            {
                double r1, r2;
                Entry r;
                pop(constants, &r);
                r2 = r.value;
                pop(constants, &r);
                r1 = r.value;
                r.value = operation(r1, r2, e.type);
                push(constants, r);
            }
        }
    }
    Entry r;
    pop(constants, &r);
    double result = r.value;
    free_stack(constants);
    free_stack(postfix);
    free_stack(op);
    free_stack(infix);
    return result;
}

// This is basically a copy paste of the quick calc in 4coder_casey.cpp
// I do the calculation differently though.
CUSTOM_COMMAND_SIG(quick_calc)
{
    View_Summary view = get_active_view(app, AccessOpen);
    Range range = get_view_range(&view);
    
    size_t length = range.max - range.min;
    char *eqn = (char*)malloc(sizeof(char) * (length + 1));
    
    Buffer_Summary buffer = get_buffer(app, view.buffer_id, AccessOpen);
    buffer_read_range(app, &buffer, range.min, range.max, eqn);
    double result = solve_equation(eqn, length);
    
    char result_buffer[256];
    int result_size = sprintf(result_buffer, "%f", result);
    
    buffer_replace_range(app, &buffer, range.min, range.max, result_buffer, result_size);
    free(eqn);
}

// Leader key type thing. Add any other <leader><_> type commands to the switch statement.
CUSTOM_COMMAND_SIG(leader_key_query)
{
    switch(get_user_input(app, EventOnAnyKey, EventOnEsc).key.keycode)
    {
        case ' ':
        {
            change_active_panel(app);
        }
        break;
        
        // Open file in other panel if one exists. If not opens a new panel and opens a file in it
        case 'v':
        {
            View_Summary view = get_active_view(app, AccessAll);
            View_Summary next_view = get_next_view_after_active(app, AccessAll);
            View_ID original = view.view_id;
            View_ID next = next_view.view_id;
            
            if(original == next)
            {
                View_Summary new_view = open_view(app, &view, ViewSplit_Right);
                new_view_settings(app, &new_view);
                view_set_buffer(app, &new_view, view.buffer_id, 0);
            }
            else
            {
                change_active_panel(app);
            }
            exec_command(app, interactive_open_or_new);
        }
        break;
        
        case 'r':
        {
            reopen(app);
        }
        break;
    }
}

// Auto completes an open curly brace in different ways depending on the next key pressed.
CUSTOM_COMMAND_SIG(curly_command_query)
{
    switch(get_user_input(app, EventOnAnyKey, EventOnEsc).key.keycode)
    {
        case '\n':
        {
            write_string(app, make_lit_string("{\n\n}"));
            vim_move_up(app);
        } break;
        
        case '}':
        {
            write_string(app, make_lit_string("{}"));
            vim_move_left(app);
        } break;
        
        case ';':
        {
            write_string(app, make_lit_string("{\n\n};"));
            vim_move_up(app);
        } break;
    }
}

// Basic auto complete stuff
CUSTOM_COMMAND_SIG(close_open_bracket)
{
    write_string(app, make_lit_string("()"));
    vim_move_left(app);
}

CUSTOM_COMMAND_SIG(close_open_bracket_square)
{
    write_string(app, make_lit_string("[]"));
    vim_move_left(app);
}

CUSTOM_COMMAND_SIG(close_open_quote)
{
    write_string(app, make_lit_string("\"\""));
    vim_move_left(app);
}

CUSTOM_COMMAND_SIG(system_clipboard_paste)
{
    View_Summary view = get_active_view(app, AccessOpen);
    Buffer_Summary buffer = get_buffer(app, view.buffer_id, AccessOpen);
    int pos = view.cursor.pos;
    paste_from_register(app, &buffer, pos, &state.registers[reg_system_clipboard]);
}

CUSTOM_COMMAND_SIG(auto_todo)
{
    write_string(app, make_lit_string("// TODO(Luke): "));
}

CUSTOM_COMMAND_SIG(auto_note)
{
    write_string(app, make_lit_string("// NOTE(Luke): "));
}

// Center search results on the screen
CUSTOM_COMMAND_SIG(search_and_center)
{
    vim_search(app);
    center_view(app);
}

CUSTOM_COMMAND_SIG(search_and_center_next)
{
    vim_search_next(app);
    center_view(app);
}

CUSTOM_COMMAND_SIG(search_and_center_prev)
{
    vim_search_prev(app);
    center_view(app);
}

// Command for jumping to the end of common enclosing characters.
CUSTOM_COMMAND_SIG(seek_next_closing)
{
    View_Summary view = get_active_view(app, AccessOpen);
    Buffer_Summary buffer = get_buffer(app, view.buffer_id, AccessOpen);
    
    char cur;
    int pos = view.cursor.pos;
    while(pos < buffer.size && 0 < buffer.size)
    {
        buffer_read_range(app, &buffer, pos, pos + 1, &cur);
        if(cur == ')' || cur == '}' || cur == '"' || cur == ']' || cur == '"')
        {
            view_set_cursor(app, &view, seek_pos(pos + 1), 1);
            break;
        }
        ++pos;
    }
}

CUSTOM_COMMAND_SIG(jump_under_cursor)
{
    View_Summary view = get_active_view(app, AccessAll);
    Buffer_ID buffer_id = view.buffer_id;
    Buffer_Summary buffer = get_buffer(app, buffer_id, AccessAll);
    
    Marker_List *list = get_or_make_list_for_buffer(app, &global_part, &global_heap, buffer_id);
    
    Temp_Memory temp = begin_temp_memory(&global_part);
    i32 option_count = list->jump_count;
    Managed_Object stored_jumps = list->jump_array;
    for(i32 i = 0; i < option_count; i++)
    {
        Sticky_Jump_Stored stored = {};
        managed_object_load_data(app, stored_jumps, i, 1, &stored);
        String line = {};
        read_line(app, &global_part, &buffer, stored.list_line, &line);
        write_string(app, line);
    }
    end_temp_memory(temp);
}

CUSTOM_COMMAND_SIG(visual_upper_case)
{
    View_Summary view = get_active_view(app, AccessOpen);
    view_set_cursor(app, &view, seek_pos(state.selection_range.end), 1);
    view_set_mark(app, &view, seek_pos(state.selection_range.start));
    to_uppercase(app);
    enter_normal_mode(app, view.buffer_id);
}

CUSTOM_COMMAND_SIG(visual_place_in_scope)
{
    View_Summary view = get_active_view(app, AccessOpen);
    view_set_cursor(app, &view, seek_pos(state.selection_range.end), 1);
    view_set_mark(app, &view, seek_pos(state.selection_range.start));
    place_in_scope(app);
    enter_normal_mode(app, view.buffer_id);
}

CUSTOM_COMMAND_SIG(visual_surround_brackets)
{
    View_Summary view = get_active_view(app, AccessOpen);
    view_set_cursor(app, &view, seek_pos(state.selection_range.end), 1);
    view_set_mark(app, &view, seek_pos(state.selection_range.start));
    write_string(app, make_lit_string(")"));
    cursor_mark_swap(app);
    write_string(app, make_lit_string("("));
    enter_normal_mode(app, view.buffer_id);
}

CUSTOM_COMMAND_SIG(visual_replace_in_range)
{
    View_Summary view = get_active_view(app, AccessOpen);
    view_set_cursor(app, &view, seek_pos(state.selection_range.end), 1);
    view_set_mark(app, &view, seek_pos(state.selection_range.start));
    replace_in_range(app);
    enter_normal_mode(app, view.buffer_id);
}

void luke_get_bindings(Bind_Helper *context) 
{
    // Set the hooks
    set_start_hook(context, luke_init);
    set_open_file_hook(context, vim_hook_open_file_func);
    set_new_file_hook(context, vim_hook_new_file_func);
    set_render_caller(context, vim_render_caller);
    
    // Call to set the vim bindings
    vim_get_bindings(context);
    
    // Since keymaps are re-entrant, I can define my own keybindings below
    // here that will apply in the appropriate map:
    
    begin_map(context, mapid_movements);
    // For example, I forget to hit shift a lot when typing commands. Since
    // semicolon doesn't do much useful in vim by default, I bind it to the
    // same command that colon itself does.
    bind(context, ';', MDFR_NONE, status_command);
    bind(context, 'j', MDFR_CTRL, vim_move_whitespace_down);
    bind(context, 'k', MDFR_CTRL, vim_move_whitespace_up);
    bind(context, 'L', MDFR_NONE, vim_move_end_of_line);
    bind(context, 'H', MDFR_NONE, vim_move_beginning_of_line);
    bind(context, 'z', MDFR_NONE, center_view);
    bind(context, 'n', MDFR_NONE, search_and_center_next);
    bind(context, 'N', MDFR_NONE, search_and_center_prev);
    bind(context, '/', MDFR_NONE, search_and_center);
    end_map(context);
    
    begin_map(context, mapid_insert);
    bind(context, '{', MDFR_NONE, curly_command_query);
    bind(context, '(', MDFR_NONE, close_open_bracket);
    bind(context, '[', MDFR_NONE, close_open_bracket_square);
    bind(context, '"', MDFR_NONE, close_open_quote);
    bind(context, 'j', MDFR_CTRL, seek_next_closing);
    bind(context, 't', MDFR_CTRL, auto_todo);
    bind(context, 'e', MDFR_CTRL, auto_note);
    end_map(context);
    
    begin_map(context, mapid_normal);
    bind(context, ' ', MDFR_NONE, leader_key_query);
    bind(context, 'q', MDFR_NONE, system_clipboard_paste);
    bind(context, 's', MDFR_NONE, quick_calc);
    bind(context, '[', MDFR_NONE, jump_under_cursor);
    end_map(context);
    
    begin_map(context, mapid_visual);
    bind(context, 's', MDFR_NONE, visual_replace_in_range);
    bind(context, '~', MDFR_NONE, visual_upper_case);
    bind(context, '{', MDFR_NONE, visual_place_in_scope);
    bind(context, '(', MDFR_NONE, visual_surround_brackets);
    end_map(context);
    
    // I can also define custom commands very simply:
    
    // As an example, suppose we want to be able to use 'save' to write the
    // current file:
    define_command(make_lit_string("save"), write_file);
    define_command(make_lit_string("W"), write_file);
    // (In regular vim, :saveas is a valid command, but this hasn't yet
    // been defined in the 4coder vim layer. If it were, this definition 
    // would be pointless, as :save would match as a substring of :saveas 
    // first.)
    
    // TODO(chr): Make the statusbar commands more intelligent
    //  so that this isn't an issue.
}

extern "C" int
get_bindings(void *data, int size) {
    Bind_Helper context_ = begin_bind_helper(data, size);
    Bind_Helper *context = &context_;
    
    luke_get_bindings(context);
    
    int result = end_bind_helper(context);
    return(result);
}
