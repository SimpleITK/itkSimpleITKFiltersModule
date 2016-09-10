#! /usr/bin/env bash

env

mkdir -p "${ExternalData_OBJECT_STORES}"

if [ -n "${APPVEYOR_BUILD_FOLDER+x}" ]
then
    PROJ_SRC=${APPVEYOR_BUILD_FOLDER}
fi

( cd ${ExternalData_OBJECT_STORES} &&
    if [[ ! -e ${ITK_REPOSITORY} ]]
    then
        git clone --mirror ${ITK_REPOSITORY_REMOTE}
    fi &&
    cd ${ITK_REPOSITORY} &&
    git remote update
)

git clone --single-branch ${ITK_REPOSITORY} -b "${ITK_TAG}" "${ITK_SRC}" &&
 ( cd ${ITK_SRC}/Modules/Remote &&
     ln -s ${PROJ_SRC} ${ITK_MODULE_NAME}
 )

( cd ${PROJ_SRC}/test &&
    git clone --single-branch ${ITK_REPOSITORY} -b dashboard dashboard
)
