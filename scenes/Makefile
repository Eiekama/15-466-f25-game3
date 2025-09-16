.PHONY : all

#n.b. the '-y' sets autoexec scripts to 'on' so that driver expressions will work
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	BLENDER = /Applications/Blender.app/Contents/MacOS/Blender -y
else ifeq ($(UNAME_S),Linux)
	BLENDER = ../../blender-4.5.2-linux-x64/blender -y
else
	BLENDER = blender
endif

EXPORT_MESHES=export-meshes.py
EXPORT_SCENE=export-scene.py

DIST=../dist

all : \
	$(DIST)/game.pnct \
	$(DIST)/game.scene \


$(DIST)/game.scene : game.blend $(EXPORT_SCENE)
	$(BLENDER) --background --python $(EXPORT_SCENE) -- '$<':Scene '$@'

$(DIST)/game.pnct : game.blend $(EXPORT_MESHES)
	$(BLENDER) --background --python $(EXPORT_MESHES) -- '$<':Collection '$@'
