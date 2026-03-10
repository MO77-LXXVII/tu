// bank.hpp

#pragma once

// umbrella header: includes all bank headers

#include "entities/person.hpp"
#include "entities/bank_client.hpp"
#include "entities/bank_user.hpp"
#include "entities/transaction_log.hpp"
#include "persistence/persistent_entity.hpp"
#include "persistence/file_cache.hpp"
#include "services/currency_exchange.hpp"
