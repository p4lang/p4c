;;; p4-mode.el --- Support for the P4_16 programming language
;;; This file is still incomplete

;; Copyright (C) 2016- Barefoot Networks
;; Version: 1.2
;; Keywords: languages p4
;; Homepage: http://p4.org

;; This file is not part of GNU Emacs.
;; This file is free software…

;; Placeholder for user customization code
(defvar p4-mode-hook nil)

;; Define the keymap (for now it is pretty much default)
(defvar p4-mode-map
  (let ((map (make-keymap)))
    (define-key map "\C-j" 'newline-and-indent)
    map)
  "Keymap for P4 major mode")

;; Syntactic HighLighting

;; Main keywors (declarations and operators)
(setq p4-keywords
      '("action" "apply" "const" "control"
        "else" "extern" "if" "parser" "package"
        "return" "select" "state" "switch" "table"
        "transition" "typedef" "type" "value_set"
        ))

(setq p4-attributes
      '("actions" "in" "inout" "key" "out"
        ))

(setq p4-constants
      '("false" "true" "default"))

(setq p4-types
      '("bool" "bit" "enum" "error" "header" "header_union"
        "int" "match_kind" "struct" "varbit" "void"))

(setq p4-cpp
      '("#include"
        "#define" "#undef"
        "#if" "#ifdef" "#elif" "#else" "#endif" "defined"
        "#line"))

(setq p4-cppwarn
      '("error" "warning"))

;; Optimize the strings
(setq p4-keywords-regexp   (regexp-opt p4-keywords   'words))
(setq p4-attributes-regexp (regexp-opt p4-attributes 'words))
(setq p4-constants-regexp  (regexp-opt p4-constants  'words))
(setq p4-types-regexp      (regexp-opt p4-types      'words))
(setq p4-cpp-regexp        (regexp-opt p4-cpp        'words))
(setq p4-cppwarn-regexp    (regexp-opt p4-cppwarn    'words))


;; create the list for font-lock.
;; each category of keyword is given a particular face
(defconst p4-font-lock-keywords
  (list
   (cons p4-types-regexp       font-lock-type-face)
   (cons p4-constants-regexp   font-lock-constant-face)
   (cons p4-attributes-regexp  font-lock-builtin-face)
   (cons p4-keywords-regexp    font-lock-keyword-face)
   (cons p4-cpp-regexp         font-lock-preprocessor-face)
   (cons p4-cppwarn-regexp     font-lock-warning-face)
   (cons "\\('\\w*'\\)"        font-lock-variable-name-face)
   )
  "Default Highlighting Expressions for P4")

(defvar p4-mode-syntax-table
  (let ((st (make-syntax-table)))
    (modify-syntax-entry  ?_  "w"      st)
    (modify-syntax-entry  ?/  ". 124b" st)
    (modify-syntax-entry  ?*  ". 23"   st)
    (modify-syntax-entry  ?\n  "> b"   st)
    st)
  "Syntax table for p4-mode")

;;; Indentation
(defvar p4-indent-offset 4
  "Indentation offset for `p4-mode'.")

(defun p4-indent-line ()
  "Indent current line for any balanced-paren-mode'."
  (interactive)
  (let ((indent-col 0)
        (indentation-increasers "[{]")
        (indentation-decreasers "[}]")
        )
    (save-excursion
      (beginning-of-line)
      (condition-case nil
          (while t
            (backward-up-list 1)
            (when (looking-at indentation-increasers)
              (setq indent-col (+ indent-col p4-indent-offset))))
        (error nil)))
    (save-excursion
      (back-to-indentation)
      (when (and (looking-at indentation-decreasers) (>= indent-col p4-indent-offset))
        (setq indent-col (- indent-col p4-indent-offset))))
    (indent-line-to indent-col)))


;; Put everything together
(defun p4-mode ()
  "Major mode for editing P4 v1.2 programs"
  (interactive)
  (kill-all-local-variables)
  (set-syntax-table p4-mode-syntax-table)
  (use-local-map p4-mode-map)
  (set (make-local-variable 'font-lock-defaults) '(p4-font-lock-keywords))
  (set (make-local-variable 'indent-line-function) 'p4-indent-line)
  (setq major-mode 'p4-mode)
  (setq mode-name "P4")
  (run-hooks 'p4-mode-hook)
)

;; The most important line
(provide 'p4-mode)
