#include "ovpch.h"
#include "Physics.h"
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

namespace Oeuvre
{
	void Physics::Init()
	{
		// Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
		// This needs to be done before any other Jolt function is called.
		RegisterDefaultAllocator();

		// Install trace and assert callbacks
		Trace = TraceImpl;
		JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

			// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
			// It is not directly used in this example but still required.
			Factory::sInstance = new Factory();

		// Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
		// If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
		// If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
		RegisterTypes();

		// We need a temp allocator for temporary allocations during the physics update. We're
		// pre-allocating 10 MB to avoid having to do allocations during the physics update.
		// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
		// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
		// malloc / free.
		//TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
		temp_allocator = new TempAllocatorImpl(10 * 1024 * 1024);;

		// We need a job system that will execute physics jobs on multiple threads. Typically
		// you would implement the JobSystem interface yourself and let Jolt Physics run on top
		// of your own job scheduler. JobSystemThreadPool is an example implementation.
		//JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
		job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

		// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
		// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
		const uint cMaxBodies = 1024;

		// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
		const uint cNumBodyMutexes = 0;

		// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
		// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
		// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
		// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
		const uint cMaxBodyPairs = 1024;

		// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
		// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
		// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
		const uint cMaxContactConstraints = 1024;

		// Create mapping table from object layer to broadphase layer
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		// Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
		//BPLayerInterfaceImpl broad_phase_layer_interface;

		// Create class that filters object vs broadphase layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		// Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
		//ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

		// Create class that filters object vs object layers
		// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
		// Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
		//ObjectLayerPairFilterImpl object_vs_object_layer_filter;

		// Now we can create the actual physics system.
		// PhysicsSystem physics_system;
		physics_system = std::make_unique<PhysicsSystem>();
		physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

		// A body activation listener gets notified when bodies activate and go to sleep
		// Note that this is called from a job so whatever you do here needs to be thread safe.
		// Registering one is entirely optional.
		//MyBodyActivationListener body_activation_listener;
		physics_system->SetBodyActivationListener(&body_activation_listener);

		// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
		// Note that this is called from a job so whatever you do here needs to be thread safe.
		// Registering one is entirely optional.
		//MyContactListener contact_listener;
		physics_system->SetContactListener(&contact_listener);

		physics_system->SetGravity(m_Gravity);

		// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
		// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
		BodyInterface& body_interface = physics_system->GetBodyInterface();

		// Next we can create a rigid body to serve as the floor, we make a large box
		// Create the settings for the collision volume (the shape).
		// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
		BoxShapeSettings floor_shape_settings(Vec3(10000.0f, 1.0f, 10000.0f));
		floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

		// Create the shape
		ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
		ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

		// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
		BodyCreationSettings floor_settings(floor_shape, RVec3(0.0_r, -1.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
		floor_settings.mFriction = 0.9f;

		// Create the actual rigid body
		//Body* floor = body_interface->CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr
		floor = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

		// Add it to the world
		body_interface.AddBody(floor->GetID(), EActivation::DontActivate);

		// Now create a dynamic body to bounce on the floor
		// Note that this uses the shorthand version of creating and adding a body to the world
		BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0_r, 20.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
		//BodyID sphere_id = body_interface->CreateAndAddBody(sphere_settings, EActivation::Activate);
		sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);

		// Now you can interact with the dynamic body, in this case we're going to give it a velocity.
		// (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
		body_interface.SetLinearVelocity(sphere_id, Vec3(0.0f, -5.0f, 0.0f));

		// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
		const float cDeltaTime = 1.0f / 60.0f;


		// Character creation
		CapsuleShapeSettings characterShapeSettings(1.5f, 0.2f);
		characterShapeSettings.SetEmbedded();
		ShapeSettings::ShapeResult characterShapeResult = characterShapeSettings.Create();
		ShapeRefC characterShape = characterShapeResult.Get();

		CharacterVirtualSettings characterSettings{};
		characterSettings.mShape = characterShape;
		characterSettings.mMaxSlopeAngle = DegreesToRadians(70.0f);
		//characterSettings.mSupportingVolume = Plane(Vec3::sAxisY(), -5.f);
		m_Character = std::make_unique<CharacterVirtual>(&characterSettings, Vec3(0.f, 20.f, 0.f), Quat::sIdentity(), 0, physics_system.get());

		m_Character->SetListener(this);

		// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
		// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
		// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
		physics_system->OptimizeBroadPhase();
	}

	void Physics::Update(float deltaTime)
	{
		// Next step
		++step;

		physics_system->GetBodyInterface().ActivateBody(sphere_id);
		//physics_system->GetBodyInterface().ActivateBody(m_Character->GetBodyID());

		// Output current position and velocity of the sphere
		//RVec3 position = physics_system->GetBodyInterface().GetCenterOfMassPosition(sphere_id);
		//Vec3 velocity = physics_system->GetBodyInterface().GetLinearVelocity(sphere_id);
		//std::cout << "Step " << step << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << std::endl;

		// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
		const int cCollisionSteps = std::max(1.f, std::ceilf(1.f / deltaTime / 60.f));

		//std::cout << "CollisionSteps: " << cCollisionSteps << "\n";

		// Step the world
		physics_system->Update(deltaTime, cCollisionSteps, temp_allocator, job_system);

		auto broadPhaseLayerFilter = physics_system->GetDefaultBroadPhaseLayerFilter(JPH::ObjectLayer(Layers::MOVING));
		auto layerFilter = physics_system->GetDefaultLayerFilter(JPH::ObjectLayer(Layers::MOVING));
		m_Character->ExtendedUpdate(deltaTime, m_Gravity, { .mWalkStairsStepUp = {0.0f , .12f, 0.0f}, .mWalkStairsStepForwardTest = m_Character->GetShape()->GetInnerRadius() }, broadPhaseLayerFilter, layerFilter, {}, {}, *temp_allocator);

		if (m_Character->IsSupported())// || m_ControlMovementInAir)
		{
			m_CharacterDisplacement = JPH::Vec3::sZero();
		}
	}

	void Physics::Shutdown()
	{
		//m_Character->RemoveFromPhysicsSystem();

		//// Remove the sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
		//physics_system->GetBodyInterface().RemoveBody(sphere_id);

		//// Destroy the sphere. After this the sphere ID is no longer valid.
		//physics_system->GetBodyInterface().DestroyBody(sphere_id);

		//// Remove and destroy the floor
		//physics_system->GetBodyInterface().RemoveBody(floor->GetID());
		//physics_system->GetBodyInterface().DestroyBody(floor->GetID());

		BodyIDVector bodies;
		physics_system->GetBodies(bodies);
		for (auto& body : bodies)
		{
			physics_system->GetBodyInterface().RemoveBody(body);
			physics_system->GetBodyInterface().DestroyBody(body);
		}

		delete job_system;
		delete temp_allocator;

		// Unregisters all types with the factory and cleans up the default material
		UnregisterTypes();

		// Destroy the factory
		delete Factory::sInstance;
		Factory::sInstance = nullptr;
	}

	glm::vec3 Physics::GetSpherePosition()
	{
		RVec3 pos = physics_system->GetBodyInterface().GetCenterOfMassPosition(sphere_id);
		return glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
	}

	void Physics::ThrowCube(Vec3 pos, Vec3 dir)
	{
		BoxShapeSettings shape_settings(Vec3(0.5f, 0.5f, 0.5f));
		//shape_settings.mDensity = 5.f;

		// Create the shape
		ShapeSettings::ShapeResult shape_result = shape_settings.Create();
		ShapeRefC shape = shape_result.Get();

		BodyCreationSettings settings(shape, pos, Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
		settings.mFriction = 0.9f;

		auto cubeId = physics_system->GetBodyInterface().CreateAndAddBody(settings, EActivation::Activate);

		physics_system->GetBodyInterface().SetLinearVelocity(cubeId, 50.f * dir);
	}

	void Physics::OnAdjustBodyVelocity(const JPH::CharacterVirtual* inCharacter, const JPH::Body& inBody2, JPH::Vec3& ioLinearVelocity, JPH::Vec3& ioAngularVelocity)
	{
	}

	bool Physics::OnContactValidate(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2)
	{
		return true;
	}

	void Physics::OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
	{
	}
	
	void Physics::OnContactSolve(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial* inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity)
	{
		// Don't allow the player to slide down static not-too-steep surfaces unless they are actively moving
		if (!m_AllowCharacterSliding && inContactVelocity.IsNearZero() && !inCharacter->IsSlopeTooSteep(inContactNormal))
		{
			ioNewCharacterVelocity = JPH::Vec3::sZero();
		}
	}
}