(set (make-local-variable 'compile-command)
     
)


(defun my-save-and-compile ()
  (interactive "")
  (save-buffer 0)
  (compile 
   (concat "g++ -o "
	   (file-name-sans-extension buffer-file-name)
	   " "
	   (buffer-file-name)
	   " -lglut -lGL -lSDL -lSDL_mixer"
	   " && "
	   (file-name-sans-extension buffer-file-name)
	   )
))

(defun my-save-and-compile ()
  (interactive "")
  (save-buffer 0)
  (compile "make" ) )
