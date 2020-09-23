// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef STEER_DIGITIZERWORKFLOW_SRC_TRDDIGITWRITERSPEC_H_
#define STEER_DIGITIZERWORKFLOW_SRC_TRDDIGITWRITERSPEC_H_

#include "Framework/DataProcessorSpec.h"
#include "DPLUtils/MakeRootTreeWriterSpec.h"
#include "Framework/InputSpec.h"
#include "TRDBase/Digit.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include <SimulationDataFormat/ConstMCTruthContainer.h>
#include <SimulationDataFormat/IOMCTruthContainerView.h>
#include "TRDBase/MCLabel.h"
#include "TRDWorkflow/TRDDigitWriterSpec.h"

namespace o2
{
namespace trd
{

template <typename T>
using BranchDefinition = framework::MakeRootTreeWriterSpec::BranchDefinition<T>;

o2::framework::DataProcessorSpec getTRDDigitWriterSpec(bool mctruth)
{
  using InputSpec = framework::InputSpec;
  using MakeRootTreeWriterSpec = framework::MakeRootTreeWriterSpec;
  using DataRef = framework::DataRef;

  auto getIndex = [](o2::framework::DataRef const& ref) -> size_t {
    return 0;
  };

  // callback to create branch name
  auto getName = [](std::string base, size_t index) -> std::string {
    return base;
  };

  // the callback to be set as hook for custom action when the writer is closed
  auto finishWriting = [](TFile* outputfile, TTree* outputtree) {
    outputtree->SetEntries(1);
    outputtree->Write();
    outputfile->Close();
  };

  // custom handler for labels:
  // essentially transform the input container (as registered in the branch definition) to the special output format for labels
  auto customlabelhandler = [](TBranch& branch, o2::dataformats::ConstMCTruthContainer<o2::trd::MCLabel> const& labeldata, DataRef const& ref) {
    // make the actual output object by adopting/casting the buffer
    // into a split format
    o2::dataformats::IOMCTruthContainerView outputcontainer(labeldata);
    auto tree = branch.GetTree();
    auto name = branch.GetName();
    // we need to make something ugly since the original branch is already registered with a different type
    // (communicated by Philippe Canal / ROOT team)
    branch.DeleteBaskets("all");
    // remove the existing branch and make a new one with the correct type
    tree->GetListOfBranches()->Remove(&branch);
    auto br = tree->Branch(name, &outputcontainer);
    br->Fill();
    br->ResetAddress();
  };

  auto labelsdef = BranchDefinition<o2::dataformats::ConstMCTruthContainer<o2::trd::MCLabel>>{InputSpec{"labelinput", "TRD", "LABELS"},
                                                                                              "TRDMCLabels", "labels-branch-name",
                                                                                              // this branch definition is disabled if MC labels are not processed
                                                                                              (mctruth ? 1 : 0),
                                                                                              customlabelhandler,
                                                                                              getIndex,
                                                                                              getName};

  return MakeRootTreeWriterSpec("TRDDigitWriter",
                                "trddigits.root",
                                "o2sim",
                                1,
                                // setting a custom callback for closing the writer
                                MakeRootTreeWriterSpec::CustomClose(finishWriting),
                                BranchDefinition<std::vector<o2::trd::Digit>>{InputSpec{"input", "TRD", "DIGITS"}, "TRDDigit"},
                                BranchDefinition<std::vector<o2::trd::TriggerRecord>>{InputSpec{"trinput", "TRD", "TRGRDIG"}, "TriggerRecord"},
                                std::move(labelsdef))();
}

} // end namespace trd
} // end namespace o2

#endif /* STEER_DIGITIZERWORKFLOW_SRC_TRDDIGITWRITERSPEC_H_ */
