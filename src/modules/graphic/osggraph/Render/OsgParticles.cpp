/***************************************************************************

    file        : OsgParticles.cpp
    created     : Sat May 2024 15:52:19 CEST 2024
    copyright   : (C) 2024 by MADBAD
    email       : madbad82@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <osgDB/Registry>
#include <osg/Group>

#include <tgf.h>

#include "OsgMain.h"
#include "OsgCar.h"
#include "OsgRender.h"
#include "OsgParticles.h"

namespace osggraph {

//particle systems, this will serve as a base for our particle effects
SDParticleSystem::SDParticleSystem(osg::Group *mySceneRoot, osg::Group *myAttachToObj){
    sceneRoot = mySceneRoot;
    attachToObj = myAttachToObj;
    initialize();
};
SDParticleSystem::~SDParticleSystem(void){};

void SDParticleSystem::initialize(){
    //create our own default template
    this->ptemplate.setLifeTime(1.5); // in seconds

    // the following ranges set the envelope of the respective
    // graphical properties in time.
    this->ptemplate.setSizeRange(osgParticle::rangef(0.1f, 1.5f));
    this->ptemplate.setAlphaRange(osgParticle::rangef(0.8f, 0.0f));
    this->ptemplate.setColorRange(osgParticle::rangev4(
        osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f),
        osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));

    //assign a texture
    this->particleSystem->setDefaultAttributes("data/textures/smoke2.png", false, false);

    // assign the particle template to the system.
    this->particleSystem->setDefaultParticleTemplate(this->ptemplate);
    this->emitter->setParticleSystem(this->particleSystem);
    this->randomRateCounter->setRateRange(0, 0);

    //setup the placer
    this->placer->setCenter(-1.5, 1.0, 0);
    this->placer->setRadiusRange(0.05, 0.1);
    this->placer->setPhiRange(0, 2 * osg::PI); // 360 angle to make a circle
    this->emitter->setPlacer(this->placer);

    //attach the emitter to the desired already existing graphical object (ex: wheels/cars)
    //so that the emitter will follow that position
    this->attachToObj->addChild(this->emitter);

    // now let's setup the shooter; we use a RadialShooter but we set the
    // nb(setup the shooting angles)
    this->shooter->setInitialSpeedRange(1.5, 5.0);
    this->shooter->setInitialRotationalSpeedRange(osgParticle::rangev3(
        osg::Vec3(0, 0, -1),
        osg::Vec3(0, 0, 1)));
    this->emitter->setShooter(shooter);

    //attach the geode to the main scene
    this->geode->addDrawable( this->particleSystem);
    this->sceneRoot->addChild(this->geode);
};
void SDParticleSystem::startEmitting(){
    this->isActive = true;
    this->randomRateCounter->setRateRange(100, 300);
};
void SDParticleSystem::stopEmitting(){
    this->isActive = false;
    this->randomRateCounter->setRateRange(0, 0);
}

void SDParticleSystem::setEmissionType(std::string groundMaterialType){
    if (groundMaterialType.find("sparks")!=std::string::npos){
        this->particleSystem->setDefaultAttributes("", true, false);
        osgParticle::Particle newTemplate;
        newTemplate.setLifeTime(0.3);
        newTemplate.setSizeRange(osgParticle::rangef(0.015f, 0.025f));
        newTemplate.setAlphaRange(osgParticle::rangef(1.0f, 0.9f));
        newTemplate.setColorRange(osgParticle::rangev4(
        osg::Vec4(1.0f, 0.22f, 0.0f, 0.9f),
        osg::Vec4(1.0f, 0.57f, 0.0f, 0.5f)));
        this->particleSystem->setDefaultParticleTemplate(newTemplate);
        this->particleSystem->setDefaultAttributes("", false, false);

        //set the shooting angle
        // Theta is the angle between the velocity vector and Z axis
        this->shooter->setThetaRange( osg::PI_2 - 0.01f, osg::PI_2 + 0.01f );//negatives value on the first parameter mean wider on the top of the car
                                                                            //positive value on the second parameter mean wider on the bottom of the car
        // Phi is the angle between X axis and the velocity vector projected
        // onto the XOY plane
        this->shooter->setPhiRange( -0.2f, 0.2f );//negatives value on the first parameter mean wider on the right
                                                  //positive value on the second parameter mean wider on the left
        this->shooter->setInitialSpeedRange(0.2, 0.5);

    }else if (groundMaterialType.find("sand")!=std::string::npos){
        osgParticle::Particle newTemplate;
        newTemplate.setLifeTime(3.0);
        newTemplate.setSizeRange(osgParticle::rangef(0.5f, 2.5f));
        newTemplate.setAlphaRange(osgParticle::rangef(0.4f, 0.0f));
        newTemplate.setColorRange(osgParticle::rangev4(
        osg::Vec4(0.85f, 0.62f, 0.28f, 0.7f),
        osg::Vec4(0.85f, 0.62f, 0.28f, 0.1f)));
        this->particleSystem->setDefaultParticleTemplate(newTemplate);
        this->shooter->setInitialSpeedRange(1.5, 5.0);
    }else if(groundMaterialType.find("dirt")!=std::string::npos){
        osgParticle::Particle newTemplate;
        newTemplate.setLifeTime(1.5);
        newTemplate.setSizeRange(osgParticle::rangef(0.1f, 1.5f));
        newTemplate.setAlphaRange(osgParticle::rangef(0.8f, 0.0f));
        newTemplate.setColorRange(osgParticle::rangev4(
            osg::Vec4(0.38f, 0.29f, 0.15f, 1.0f),
            osg::Vec4(0.38f, 0.29f, 0.15f, 0.1f)));
        this->particleSystem->setDefaultParticleTemplate(newTemplate);
    }else if(groundMaterialType.find("mud")!=std::string::npos){
        osgParticle::Particle newTemplate;
        newTemplate.setLifeTime(1.5);
        newTemplate.setSizeRange(osgParticle::rangef(0.1f, 1.5f));
        newTemplate.setAlphaRange(osgParticle::rangef(0.8f, 0.0f));
        newTemplate.setColorRange(osgParticle::rangev4(
            osg::Vec4(0.38f, 0.29f, 0.15f, 1.0f),
            osg::Vec4(0.38f, 0.29f, 0.15f, 0.1f)));
        this->particleSystem->setDefaultParticleTemplate(newTemplate);
    }else if(groundMaterialType.find("gravel")!=std::string::npos){
        osgParticle::Particle newTemplate;
        newTemplate.setLifeTime(1.5);
        newTemplate.setSizeRange(osgParticle::rangef(0.1f, 1.5f));
        newTemplate.setAlphaRange(osgParticle::rangef(0.8f, 0.0f));
        newTemplate.setColorRange(osgParticle::rangev4(
            osg::Vec4(0.85f, 0.62f, 0.28f, 1.0f),
            osg::Vec4(0.85f, 0.62f, 0.28f, 0.1f)));
        this->particleSystem->setDefaultParticleTemplate(newTemplate);
    }else if(groundMaterialType.find("grass")!=std::string::npos){
        osgParticle::Particle newTemplate;
        newTemplate.setLifeTime(1.5);
        newTemplate.setMass(1.0f);
        newTemplate.setSizeRange(osgParticle::rangef(0.03f, 0.05f));
        newTemplate.setAlphaRange(osgParticle::rangef(1.0f, 1.0f));
        newTemplate.setColorRange(osgParticle::rangev4(
            osg::Vec4(0.19, 0.1, 0, 1.0f),
            osg::Vec4(0.19, 0.1, 0, 1.0f)));
        this->particleSystem->setDefaultParticleTemplate(newTemplate);
        this->shooter->setInitialSpeedRange(-0.5,-0.9); // meters/second
        this->shooter->setThetaRange( osg::PI_2 - 0.01f, osg::PI_2 + 0.01f );
    }else if(groundMaterialType.find("snow")!=std::string::npos){
        osgParticle::Particle newTemplate;
        newTemplate.setLifeTime(1.5);
        newTemplate.setSizeRange(osgParticle::rangef(0.1f, 1.5f));
        newTemplate.setAlphaRange(osgParticle::rangef(0.8f, 0.0f));
        newTemplate.setColorRange(osgParticle::rangev4(
            osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
            osg::Vec4(1.0f, 1.0f, 1.0f, 0.1f)));
        this->particleSystem->setDefaultParticleTemplate(newTemplate);
    }else{ //default if no other material match found
           //asphalt)
        osgParticle::Particle newTemplate;
        newTemplate.setLifeTime(1.5);
        newTemplate.setSizeRange(osgParticle::rangef(0.1f, 0.8f));
        newTemplate.setAlphaRange(osgParticle::rangef(0.8f, 0.0f));
        newTemplate.setColorRange(osgParticle::rangev4(
            osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f),
            osg::Vec4(1.0f, 1.0f, 1.0f, 0.1f)));
        this->particleSystem->setDefaultParticleTemplate(newTemplate);
        this->shooter->setInitialSpeedRange(1.5, 5.0);
        this->shooter->setThetaRange( osg::PI_2 - 0.01f, osg::PI_2 + 0.01f );
    }

};


//particle systems, manager: this will take care of create and enabling the particles systems (effects) when needed
SDParticleSystemManager::SDParticleSystemManager(void){};
SDParticleSystemManager::~SDParticleSystemManager(void){};

void SDParticleSystemManager::initialize(tSituation *s){
    //one for each of the 4 wheels of a car
    this->tiresSmokeParticleSystems.resize(s->_ncars * 4);
    //one for each car
    this->sparksParticleSystems.resize(s->_ncars);

    //get the root node of the scene
    SDRender * renderer = getRender();
    osg::ref_ptr<osg::Group> root = renderer->getRoot();

    //add the particle system updater
    osgParticle::ParticleSystemUpdater *psu = new osgParticle::ParticleSystemUpdater;

    SDCars *cars = getCars();

    for (int h = 0; h < s->_ncars; h++){
        tCarElt* curCar = s->cars[h];
        osg::ref_ptr<osg::Group> carRoot = cars->getCar(curCar)->getCarOsgGroup();

        //add a sparks emitter for each car ??
        this->sparksParticleSystems[h] = new SDParticleSystem(root,carRoot);
        this->sparksParticleSystems[h]->setEmissionType("sparks");
        psu->addParticleSystem(sparksParticleSystems[h]->particleSystem);

         // for each wheel of this car
        for (int i = 0; i <= 3; i++){
            int index = h*4+i;
            this->tiresSmokeParticleSystems[index] = new SDParticleSystem(root,carRoot);
            this->tiresSmokeParticleSystems[index]->placer->setCenter(curCar->priv.wheel[i].relPos.x, curCar->priv.wheel[i].relPos.y, curCar->priv.wheel[i].relPos.z);

            //GfLogInfo("wheel %i x:%f y:%f z:%f\n", i, curCar->priv.wheel[i].relPos.x, curCar->priv.wheel[i].relPos.y, curCar->priv.wheel[i].relPos.z);

            psu->addParticleSystem(tiresSmokeParticleSystems[index]->particleSystem);
        }
    }

    // add the updater node to the scene graph
    root->addChild(psu);
};

void SDParticleSystemManager::update(tSituation *s){
    //update the particle systems for each car and wheel
    for (int h = 0; h < s->_ncars; h++){
        tCarElt* car = s->cars[h];
        for (int i = 0; i <= 3; i++){

            //get the ParticleSystems index for this specific wheel
            //we must use the position of the car as index because indexs get changed at runtime
            int wheelIndex = car->_startRank * 4 + i;
            //get reference of the main scene root node
            SDRender * renderer = getRender();
            osg::ref_ptr<osg::Group> root = renderer->getRoot();

            //set the particle emission type depending on what surface the wheel is touching
            //know surface at the time of this implementation are (sand,dirt,mud,gravel,grass,snow)
            std::string groundMaterialType = car->priv.wheel[i].seg->surface->material;
            this->tiresSmokeParticleSystems[wheelIndex]->setEmissionType(groundMaterialType);

            //determine if the car/wheel is skidding/sliding
            bool wheelIsSlidng = (MAX(0.0, ((car->_wheelSpinVel(i) * car->_wheelRadius(i)) - fabs(car->_speed_x)) - 9.0)) > 0.0;
            bool wheelIsSkidding =  (car->_skid[i]  > 0.4);

            //if so emit smoke/sand/dirt (always depending on the surface we are on)
            if(wheelIsSlidng || wheelIsSkidding){
                if(!this->tiresSmokeParticleSystems[wheelIndex]->isActive){
                    this->tiresSmokeParticleSystems[wheelIndex]->startEmitting();
                }
            }else{
                if(this->tiresSmokeParticleSystems[wheelIndex]->isActive){
                    this->tiresSmokeParticleSystems[wheelIndex]->stopEmitting();
                }
            }
        }

        //if a collision is happening emit sparks
        //nb the collision position passed be the simuEngine is not good yet, I should rework that
        int carIndex = car->_startRank;
        if(car->priv.collision){
                //get the ParticleSystems index for this specific wheel
                //we must use the position of the car as index because indexs get changed at runtime
                tCollisionState* collision_state = &car->priv.collision_state;
            if(!sparksParticleSystems[carIndex]->isActive){
                std::string material;
                material = "sparks";
                this->sparksParticleSystems[carIndex]->setEmissionType(material);
                this->sparksParticleSystems[carIndex]->placer->setCenter(collision_state->pos[0], collision_state->pos[1], collision_state->pos[2]);//front/rear ** right/left ** down/up of the car
                this->sparksParticleSystems[carIndex]->startEmitting();
            }
        }else{
            if(this->sparksParticleSystems[carIndex]->isActive){
                this->sparksParticleSystems[carIndex]->stopEmitting();
            }
        }
    }
};

void SDParticleSystemManager::shutdown(){
    //empty these vectors (the will be resized if needed at the next race start anyway)
    this->tiresSmokeParticleSystems.resize(0);
    this->sparksParticleSystems.resize(0);
};

} // namespace osggraph
