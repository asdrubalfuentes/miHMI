/**
 * mock_source.h  -  Fuente de datos simulada para desarrollo de la UI (Fase 1).
 */
#pragma once
#include "data_source.h"

class MockSource : public DataSource {
public:
	bool begin() override;
	void poll() override;
	bool sendCommand(const Command &cmd) override;
	bool isHealthy() const override { return true; }
	uint32_t lastOkMs() const override { return lastOk_; }
	const char *name() const override { return "Sim"; }

private:
	uint32_t last_   = 0;   /* marca de tiempo de la ultima integracion */
	uint32_t lastOk_ = 0;
};
