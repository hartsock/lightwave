/* **********************************************************************
 * Copyright 2012 VMware, Inc.  All rights reserved. 
 * *********************************************************************/
package com.vmware.identity.saml.ext.impl;

import org.opensaml.common.impl.AbstractSAMLObjectMarshaller;
import org.opensaml.xml.XMLObject;
import org.opensaml.xml.io.MarshallingException;
import org.w3c.dom.Element;

/**
 * Marshaller for RenewableType
 *
 */
public final class RenewableTypeMarshaller extends AbstractSAMLObjectMarshaller {

    @Override
    protected void marshallAttributes(XMLObject samlObject, Element domElement)
       throws MarshallingException {
       // do nothing, no attributes defined
    }

}
