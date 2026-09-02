#pragma once

// Stable public umbrella header. Implementations live in the compiled
// genesis_core library; clients can include narrower module headers instead.
#include "genesis/common/text.hpp"
#include "genesis/development/maturity.hpp"
#include "genesis/genetics/expression.hpp"
#include "genesis/genetics/genome.hpp"
#include "genesis/genetics/persistence.hpp"
#include "genesis/genetics/reproduction.hpp"
#include "genesis/identity/lineage.hpp"
#include "genesis/memory/origin.hpp"
#include "genesis/provenance/ledger.hpp"
#include "genesis/requirements/registry.hpp"
#include "genesis/runtime/runtime.hpp"
