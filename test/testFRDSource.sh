
mkdir -p data/run205690
rm -rf data/run205690/*
cp -R sample/* data/run205690
cmsRun testFRDSource_cfg.py
