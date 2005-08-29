
SOURCES = filexfer.java monitor.java
CLASSES = filexfer.class monitor.class
ARCHIVE  = filexfer.jar
MANIFEST = MANIFEST.MF

JC = javac -deprecation
JAR = jar

all: ${CLASSES} ${ARCHIVE}

test: ${CLASSES}
	appletviewer test.html

clean:
	rm -rf *.class *.jar

$(ARCHIVE): $(CLASSES) $(MANIFEST)
	$(JAR) cfm $(ARCHIVE) $(MANIFEST) $(CLASSES)

${CLASSES}: ${SOURCES}
	${JC} -O ${SOURCES}
