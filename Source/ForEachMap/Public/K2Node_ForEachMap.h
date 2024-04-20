// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_ForEachMap.generated.h"

/**
 * Node to allow iteration over map entries.
 */
UCLASS()
class FOREACHMAP_API UK2Node_ForEachMap : public UK2Node
{
	GENERATED_BODY()

public:
	
	virtual void AllocateDefaultPins() override;
	virtual void PostReconstructNode() override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	
private:

	UEdGraphPin* GetMapPin() const; 
	UEdGraphPin* GetKeyPin() const; 
	UEdGraphPin* GetValuePin() const;

	void PropagateToPin(UEdGraphPin* TargetPin, FEdGraphPinType Type, bool bPropagateValueType = false);
	void ResetPin(UEdGraphPin* TargetPin);

	void ConnectPins(UEdGraphPin* First, UEdGraphPin* Second);
	void MovePins(FKismetCompilerContext& Context, UEdGraphPin* Source, UEdGraphPin* Target);
	void CopyPins(FKismetCompilerContext& Context, UEdGraphPin* Source, UEdGraphPin* Target);

};
