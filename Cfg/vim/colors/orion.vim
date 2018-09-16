" Vim color file

" Set 'background' back to the default.  The value can't always be estimated
" and is then guessed.
hi clear Normal

" Remove all existing highlighting and set the defaults.
hi clear

" Load the syntax highlighting defaults, if it's enabled.
if exists("syntax_on")
  syntax reset
endif

" Colorscheme default
let g:colors_name="orion"

" Hardcoded colors only for console edit, but who uses GUI anyway
hi VertSplit    ctermfg=241  ctermbg=241  cterm=None
hi LineNr       ctermfg=250  ctermbg=None cterm=italic
hi StatusLine   ctermfg=114  ctermbg=241  cterm=bold,italic
hi ColorColumn  ctermfg=None ctermbg=241  cterm=None
hi StatusLineNC ctermfg=231  ctermbg=241  cterm=None
hi Pmenu        ctermfg=None ctermbg=None cterm=None
hi PmenuSel     ctermfg=None ctermbg=238  cterm=None
hi Directory    ctermfg=141  ctermbg=None cterm=None
hi Normal       ctermfg=015  ctermbg=None cterm=None
hi Search       ctermfg=000  ctermbg=011  cterm=None
hi String       ctermfg=140  ctermbg=None cterm=None
hi Character    ctermfg=140  ctermbg=None cterm=None
hi Number       ctermfg=140  ctermbg=None cterm=italic
hi Float        ctermfg=140  ctermbg=None cterm=italic
hi Boolean      ctermfg=140  ctermbg=None cterm=bold
hi Keyword      ctermfg=140  ctermbg=None cterm=bold
hi Comment      ctermfg=250  ctermbg=None cterm=None
hi Identifier   ctermfg=010  ctermbg=None cterm=bold,italic
hi Statement    ctermfg=226  ctermbg=None cterm=bold
hi PreProc      ctermfg=014  ctermbg=None cterm=bold
hi Special      ctermfg=140  ctermbg=None cterm=None
hi Cursor       ctermfg=None ctermbg=None cterm=None
hi Folded       ctermfg=None ctermbg=None cterm=bold
hi MatchParen   ctermfg=None ctermbg=006  cterm=None
hi Visual       ctermfg=000  ctermbg=010  cterm=None
hi Todo         ctermfg=000  ctermbg=114  cterm=bold
hi Underlined   ctermfg=None ctermbg=None cterm=underline
hi ErrorMsg     ctermfg=000  ctermbg=196  cterm=None
hi WarningMsg   ctermfg=000  ctermbg=214  cterm=None
hi Conditional  ctermfg=011  ctermbg=None cterm=italic
hi NonText      ctermfg=243  ctermbg=None cterm=None
hi Tag          ctermfg=197  ctermbg=None cterm=None
hi Function     ctermfg=121  ctermbg=None cterm=None

" For more information on the groups visit:
" http://vimdoc.sourceforge.net/htmldoc/syntax.html
"
" For 256 coloring:
" https://upload.wikimedia.org/wikipedia/en/1/15/Xterm_256color_chart.svg
"
" 16-bit color list:
" NR-16   NR-8    COLOR NAME 
" 0       0       Black
" 1       4       DarkBlue
" 2       2       DarkGreen
" 3       6       DarkCyan
" 4       1       DarkRed
" 5       5       DarkMagenta
" 6       3       Brown, DarkYellow
" 7       7       LightGray, LightGrey, Gray, Grey
" 8       0*      DarkGray, DarkGrey
" 9       4*      Blue, LightBlue
" 10      2*      Green, LightGreen
" 11      6*      Cyan, LightCyan
" 12      1*      Red, LightRed
" 13      5*      Magenta, LightMagenta
" 14      3*      Yellow, LightYellow
" 15      7*      White
"
