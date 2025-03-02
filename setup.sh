# Clones necessary projects
git clone https://github.com/vanhoefm/dragondrain-and-time.git
git clone https://github.com/vanhoefm/hostap-wpa3.git

# Replaces needed files
cp changes/dragonfly.c hostap-wpa3/src/common/dragonfly.c
cp changes/sae.c hostap-wpa3/src/common/sae.c
cp changes/sae.h hostap-wpa3/src/common/sae.h
cp changes/dragontime.c dragondrain-and-time/src/dragontime.c
