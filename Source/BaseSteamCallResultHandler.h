// ----------------------------------------------------------------------------
// 
// BaseSteamCallResultHandler.h
// Copyright (c) 2016 Corona Labs Inc. All rights reserved.
//
// ----------------------------------------------------------------------------

#pragma once


/**
  Abstract class whose derived "SteamCallResultHandler" class wraps a Steam "CCallResult" object,
  used to receive an async Steam operation's result data and then passing it to a given callback.

  This abstract wrapper class allows different template types of CCallResult<> to be stored
  in the same collection via polymorphism.
 */
class BaseSteamCallResultHandler
{
	public:
		/** Creates a new Steam CCallResult handler. */
		BaseSteamCallResultHandler();

		/** Unregisters the Steam CCallResult event handler and disposes of this object. */
		virtual ~BaseSteamCallResultHandler();

		/**
		  Determines if this handler is no longer waiting for a result after calling the Handle() method.
		  @return Returns true if Handle() has not been called yet or it has and the Steam result has already
		          been received by its assigned callback. This means that this object is now available to
		          handle another Steam async operation.

		          Returns false if this handler is still waiting for a result from Steam.
		 */
		bool IsNotWaitingForResult() const;

		/**
		  Determines if the Handle() method was called and this handler is currently waiting for a result
		  to be received by an async Steam operation.
		  @return Returns true if this handler is still waiting for a result from Steam.

		          Returns false if Handle() has not been called yet or it has and the Steam result has already
				  been received by its assigned callback. This means that this object is now available to
				  handle another Steam async operation.
		 */
		virtual bool IsWaitingForResult() const = 0;

		/** Aborts the last Handle() operation, unregistering its Steam result listener and assigned callback. */
		virtual void Abort() = 0;

	private:
		/** Copy constructor deleted to prevent it from being called. */
		BaseSteamCallResultHandler(const BaseSteamCallResultHandler&) = delete;

		/** Copy operator deleted to prevent it from being called. */
		void operator=(const BaseSteamCallResultHandler&) = delete;
};
