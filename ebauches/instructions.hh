struct Atome {
	enum class Genre {
		CONSTANT,
		INSTRUCTION
	};

	Type *type = nullptr;
	IdentifiantCode *ident = nullptr;

	Genre genre_atome{};

	COPIE_CONSTRUCT(Atome);

	static Atome *cree_constant();

	template <typename TypeInstruction, typename... Args>
	static TypeInstruction *cree(Args &&... args)
	{
		return TypeInstruction::cree(std::forward(args)...);
	}
};

struct Instruction : public Atome {
	enum class Genre {
		INVALIDE,

		APPEL,
		OPERATION_BINAIRE,
		OPERATION_UNAIRE,
		CHARGE_MEMOIRE,
		STOCKE_MEMOIRE,
		LABEL,
		BRANCHE,
		BRANCHE_CONDITION,
	};

	Genre genre = Genre::INVALIDE;
};

struct InstructionAppel : public Instruction {
	InstructionAppel() { genre = Instruction::Genre::APPEL; }

	int id_fonction = 0;
	kuri::tableau<Atome *> args{};

	static InstructionAppel *cree(int id_fonction, kuri::tableau<Atome *> &&args)
	{
		auto inst = memoire::loge<InstructionAppel>("InstructionAppel");
		inst->id_fonction = id_fonction;
		inst->args = std::move(args);
		return inst;
	}
};

struct InstructionOpBinaire : public Instruction {
	InstructionOpBinaire() { genre = Instruction::Genre::OPERATION_BINAIRE; }

	int type_op = 0;
	void *operande_gauche = nullptr;
	void *operande_droite = nullptr;

	COPIE_CONSTRUCT(Instruction);

	static InstructionOpBinaire *cree(int type_op, void *operande_gauche, void *operande_droite)
	{
		auto inst = memoire::loge<InstructionOpBinaire>("InstructionOpBinaire");
		inst->type_op = type_op;
		inst->operande_gauche = operande_gauche;
		inst->operande_droite = operande_droite;
		return inst;
	}
};

// id := op_unaire id
struct InstructionOpUnaire : public Instruction {
	InstructionOpUnaire() { genre = Instruction::Genre::OPERATION_UNAIRE; }

	int type_op = 0;
	void *operande = nullptr;

	COPIE_CONSTRUCT(Instruction);

	static InstructionOpUnaire *cree(int type_op, void *operande)
	{
		auto inst = memoire::loge<InstructionOpUnaire>("InstructionOpUnaire");
		inst->type_op = type_op;
		inst->operande = operande;
		return inst;
	}
};

// id := M[Atome]
struct InstructionChargeMem : public Instruction {
	InstructionChargeMem() { genre = Instruction::Genre::CHARGE_MEMOIRE; }

	static InstructionChargeMem *cree()
	{
		auto inst = memoire::loge<InstructionChargeMem>("InstructionChargeMem");
		return inst;
	}
};

// M[Atome] := id
struct InstructionStockeMem : public Instruction {
	InstructionStockeMem() { genre = Instruction::Genre::STOCKE_MEMOIRE; }

	static InstructionStockeMem *cree()
	{
		auto inst = memoire::loge<InstructionStockeMem>("InstructionStockeMem");
		return inst;
	}
};

// LABEL id
struct InstructionLabel : public Instruction {
	InstructionLabel() { genre = Instruction::Genre::LABEL; }

	int id = 0;

	static InstructionLabel *cree(int id)
	{
		auto inst = memoire::loge<InstructionLabel>("InstructionLabel");
		inst->id = 0;
		return inst;
	}
};

// GOTO label
struct InstructionBranche : public Instruction {
	InstructionBranche() { genre = Instruction::Genre::BRANCHE; }

	InstructionLabel *label = nullptr;

	COPIE_CONSTRUCT(InstructionBranche);

	static InstructionBranche *cree(InstructionLabel *label)
	{
		auto inst = memoire::loge<InstructionBranche>("InstructionBranche");
		inst->label = label;
		return inst;
	}
};

// IF condition THEN label ELSE label
struct InstructionBrancheCondition : public Instruction {
	InstructionBrancheCondition() { genre = Instruction::Genre::BRANCHE_CONDITION; }

	Atome *condition = nullptr;
	InstructionLabel *label_si_vrai = nullptr;
	InstructionLabel *label_si_faux = nullptr;

	COPIE_CONSTRUCT(InstructionBrancheCondition);

	static InstructionBrancheCondition *cree(Atome *condition, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux)
	{
		auto inst = memoire::loge<InstructionBrancheCondition>("InstructionBrancheCondition");
		inst->condition = condition;
		inst->label_si_vrai = label_si_vrai;
		inst->label_si_faux = label_si_faux;
		return inst;
	}
};

struct ConstructriceRI {
	kuri::tableau<Instruction *> instructions{};

	Atome *cree_fonction();

	Atome *cree_branche();
};


