//=============================================================================
// >>> my vim-based 4coder custom <<<
// author: chr <chr@chronal.net>
//
// Sample usage of vim functions, from my own 4coder custom. 
// Feel free to copy and tweak as you like!
//=============================================================================

#include "4coder_vim.cpp"

// These colors are tuned to work with the Dragonfire color scheme.
// TODO(chr): How to best make this configurable? Can we query for arbitrary
// variables in the theme?
constexpr int_color color_margin_normal = 0xFF341313;
constexpr int_color color_margin_insert = 0xFF5a3619;
constexpr int_color color_margin_replace = 0xFF5a192e;
constexpr int_color color_margin_visual = 0xFF722b04;

START_HOOK_SIG(luke_init) {
    default_4coder_initialize(app);
    exec_command(app, open_panel_vsplit);
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

CUSTOM_COMMAND_SIG(close_open_bracket_curly)
{
    write_string(app, make_lit_string("{\n\n}"));
    vim_move_up(app);
}

CUSTOM_COMMAND_SIG(close_open_bracket)
{
    write_string(app, make_lit_string("()"));
    vim_move_left(app);
}

void luke_get_bindings(Bind_Helper *context) {
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
    end_map(context);

    begin_map(context, mapid_insert);
    bind(context, '{', MDFR_NONE, close_open_bracket_curly);
    bind(context, '(', MDFR_NONE, close_open_bracket);
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
