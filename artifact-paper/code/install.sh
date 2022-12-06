$ chmod 600 pal_rsa
$ GIT_SSH_COMMAND='ssh -i pal_rsa -o IdentitiesOnly=yes' git clone git@bitbucket.org:jinb-park/pal-ae.git
