# firmware-hls : HLS implementation of the hybrid L1 track firmware

## Repo directory contents:

- IntegrationTests/ : HDL top files and simulations for chains of HLS modules and memories.
- TestBenches/ : Test bench code.
- TrackletAlgorithm/ : Algo source code.
- TrackQuality/ : BDT Track Quality specific source code.
- emData/ : .dat files with input/output test-bench data (corresponding to memory between algo steps) + .tab files of data for LUTs used internally by algos.
- project/ : .tcl scripts to create HLS project, compile & run code.

> Most of the following directions will reference Vivado HLS and the corresponding `vivado_hls` command. You may use Vitis HLS instead by replacing all of the `vivado_hls` commands with `vitis_hls`.

An HLS project can be generated by running tcl file with Vivado/Vitis HLS in firmware-hls/project/ directory. e.g. To do so for the ProjectionRouter:

        vivado_hls -f script_PR.tcl

This would create a project directory \<project> ("projrouter" in case of the above example). The project name is defined in the tcl script. To open the project in GUI:

        vivado_hls -p <project>

## Running chains (illustrated for PR-ME-MC)

1) cd IntegrationTests/PRMEMC/script/
2) vivado_hls -f compileHLS.tcl (makes HLS IP cores).
3) vivado -mode batch -source makeProject.tcl (creates Vivado project).
4) vivado -mode batch -source runSim.tcl (runs Vivado simulation,
   which writes data output from chain to dataOut/*.txt).
5) python ../../common/script/CompareMemPrintsFW.py -p -s (compares .txt files in emData and dataOut/ writing comparison to dataOut/*_cmp.txt. Uses Python 3.).
6) vivado -mode batch -source ../../common/script/synth.tcl (runs synthesis, writes utilization & timing reports to current directory).
7) vivado -mode batch -source ../../common/script/impl.tcl (runs implementation, writes utilization & timing reports to current directory). N.B. This step is optional, and not required for code validation.

## Track Quality Specific Instructions
In the TrackQuality directory first run: 

        setupEnv.sh environment.yml Install


This will install a miniconda python environment under the Install dir within the TrackQuality dir (or manually install the packages listed in environment.yml if you have an existing conda install)

Then run:

        source env.sh

to setup the python environment

To first download the testbench .dat files and the python saved model and then generate the track quality HLS from a python saved model format run:

        run_TQ_coniferconversion.sh


Once run this will generate the 2 HLS files for the track quality firmware (BDT.h and parameters.h) and a reference datafile that will be compared with the csim output. To run the track quality csim, synth etc.. :

        vivado_hls -f script_TQ.tcl

Which generates the project folder trackquality

Note on .dat testbench files:
These testbench files will be generated by the CMSSW emulation. Two files are created, one hls_feature.dat used by conifer to run its own predictions and one hls_hex.dat that contains full 96-bit tttrack_trackwords for the track quality HLS to predict on. This CMSSW emulation relies on changes to the TTTrack_trackword dataformat and as such is currently not part of the cms-L1TK CMSSW fork. A development version of CMSSW to generate the test files is found here: https://github.com/Chriisbrown/cmssw/tree/cbrown-TrackQualityMemfiles   

## Format of emData/ files.

### .dat files (test bench input/output data)

These have test data corresponding to the contents of the memories between algo steps. Their data format is explained 
in https://twiki.cern.ch/twiki/bin/view/CMS/HybridDataFormat . 

e.g. AllStubs*.dat contains one row per stub: "stub_number stub_coords_(binary)[r|z|phi|...] ditto_but_in_hex"; StubPairs*.dat contains one row per stub pair "pair_number stub_index_in_allstubs_mem_(binary)[innerLayer|outerLayer] ditto_but_in_hex.

File naming convention: "L3" or "D5" indicate barrel or disk number; "PHIC" indicates 3rd course phi division given layer of nonant.

Some of the files are large, so not stored directly in git. These are automatically downloaded when any of the scripts in the project/ directory are executed within Vivado/Vitis HLS.

### .tab files 

These correspond to LUT used internally by the algo steps.

## Corresponding CMSSW L1 track emulation

The files that are downloaded by emData/download.sh were created by the CMSSSW L1 track emulation, with the the following recipe (adapted from the [L1TrackSoftware TWiki](https://twiki.cern.ch/twiki/bin/view/CMS/L1TrackSoftware)).

```bash
cmsrel CMSSW_12_0_0_pre4
cd CMSSW_12_0_0_pre4/src/
cmsenv
git cms-checkout-topic -u cms-L1TK:fw_synch_220106
```

A few cfg changes were made in order to output test vectors & lookup tables, adjust truncation, and to disable multiple matches in the MatchCalculator. This required editing parameter values in L1Trigger/TrackFindingTracklet/interface/Settings.h to match the following excerpts:

```c++
…
    //IR should be set to 108 to match the FW for the summer chain, but ultimately should be at 156
    std::unordered_map<std::string, unsigned int> maxstep_{{"IR", 108},  //IR will run at a higher clock speed to handle
                                                                         //input links running at 25 Gbits/s
                                                                         //Set to 108 to match firmware project 240 MHz clock
…
    //--- These used to create files needed by HLS code.
    bool writeMem_{true};     //If true will print out content of memories (between algo steps) to files
    bool writeTable_{true};   //If true will print out content of LUTs to files
    bool writeConfig_{true};  //If true will print out the autogenerated configuration as files
…
    bool writeHLSInvTable_{true};  //Write out tables of drinv and invt in tracklet calculator for HLS module
…
    // When false, match calculator does not save multiple matches, even when doKF=true.
    // This is a temporary fix for compatibilty with HLS. We will need to implement multiple match
    // printing in emulator eventually, possibly after CMSSW-integration inspired rewrites
    // Use false when generating HLS files, use true when doing full hybrid tracking
    bool doMultipleMatches_{false};
…
```

In addition, if you'd like to run the Summer Chain configuration, you can do this by setting
```c++
…
    bool reduced_{true};        // use reduced (Summer Chain) config
…
```

Alternatively, if you would like to run the configuration with the combined modules (i.e., the TrackletProcessor and MatchProcessor), you can do this by setting
```c++
…
    bool combined_{true};       // use combined TP (TE+TC) and MP (PR+ME+MC) configuration
…
```

Then compilation was done with the usual command:

```bash
scram b -j8
```

Finally, the maximum number of events in L1Trigger/TrackFindingTracklet/test/L1TrackNtupleMaker_cfg.py was set to 100:

```python
…
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(100))
…
```

and the emulation run:

```bash
cd L1Trigger/TrackFindingTracklet/test/
cmsRun L1TrackNtupleMaker_cfg.py
```

## Continuous Integration (CI) 

Purpose: Automatically run SW quality checks and build the HLS projects (csim, csynth, cosim, and export) for a PR to the master.

In order to keep the GitHub repository public we use GitHub Actions and GitLab CI/CD:

* GitHub Actions uses a public runner, the workflow is defined in .github/workflows/GitLab_CI.yml
* GitHub Actions mirrors the repository to GitLab and runs GitLab CI/CD
* GitLab CI/CD uses a private runner (lnxfarm327.colorado.edu) and performs the SW quality checks and the HLS builds as defined in .gitlab-ci.yml
    - SW quality checks are based on clang-tidy (llvm-toolset-7.0) and are defined in .clang-tidy and .clang-format very similar to CMSSW
    - HLS builds are using Vivado HLS (or Vitis HLS) and are defined in the script files of the project folder
    - Results (logs and artifacts) of the SW quality checks and HLS builds can be found here https://gitlab.cern.ch/cms-l1tk/firmware-hls/pipelines
    - The default behavior blocks a stage (e.g. Hls-build) when a previous stage (e.g. Quality-check) failed 
* GitHub Actions pulls the GitLab CI/CD status and the pass/fail outcome

### Use CI for Personal Branch

* Add your branch name to the "on:" section of .github/workflows/GitLab_CI.yml 
    - In the "push:" subsection to trigger CI on each push, e.g. "branches: [feat_CI,<your_branch_name>]" and/or
    - in the "pull_request:" subsection to trigger CI on each PR, e.g. "branches: [master,<your_branch_name>]"
