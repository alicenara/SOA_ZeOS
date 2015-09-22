# INSTALLDIR tiene que apuntar al directorio donde este instalado el bochs 
#!/bin/bash
export INSTALLDIR=${HOME}/SOA_ZeOS/mybochs
export BXSHARE=${INSTALLDIR}/share/bochs
export PATH=${INSTALLDIR}/bin:${PATH}

setenv INSTALLDIR ${HOME}/SOA_ZeOS/mybochs
setenv BXSHARE ${INSTALLDIR}/share/bochs
setenv PATH ${INSTALLDIR}/bin:${PATH}
